#pragma once


#include <memory>
#include <vector>
#include <unordered_map>

#include "../my_std/my_tuple/tuple.h"
#include "../my_std/my_utility/utility.h"
#include "../my_std/my_type_traits/type_traits.h"
#include "../my_std/my_exception/exceptions.h"
#include "../my_std/my_functional/functional.h"


enum class EState {
    NotStarted,
    Running,
    Done
};


struct TaskBase : std::enable_shared_from_this<TaskBase> {
    virtual void Execute() = 0;
    virtual ~TaskBase() = default;
    virtual const std::vector<TaskBase*>& GetAddiction() const = 0;
    virtual void ClearSchedulerPointer() {}
};


template<typename T>
struct TState {
    bool ready = false;
    bool moved = false;
    T value;
};

template<>
struct TState<void> {
    bool ready = false;
    bool moved = false;
};



template<typename T>
class TFuture;

template<typename>
struct IsFuture : my_std::false_type {};

template<typename T>
struct IsFuture<TFuture<T>> : my_std::true_type {};

template<typename T>
inline constexpr bool is_future_v = IsFuture<T>::value;


template<typename T>
struct UnwrapFuture { 
    using type = T; 
};

template<typename T>
struct UnwrapFuture<TFuture<T>> { 
   using type = T; 
};

template<typename T>
using unwrap_future_t = typename UnwrapFuture<T>::type;



template<typename T>
class TFuture {
private:
    using ValueType = my_std::remove_cvref_t<T>;

    std::shared_ptr<TState<ValueType>> state_;
    std::shared_ptr<TaskBase> producer_;

public:
    explicit TFuture(std::shared_ptr<TState<ValueType>> state, std::shared_ptr<TaskBase> producer = nullptr) 
            : state_(state)
            , producer_(producer) 
    {}

    bool Ready() const {
        return state_ && state_->ready;
    }

    T get() {
        if (!Ready()) {
            throw ttask::FutureIsNotReady();
        }

        if constexpr (my_std::is_void_v<T>) {
            return;
        }
        else if constexpr (!my_std::is_lvalue_ref_v<T>) {
            if (state_->moved) {
                throw ttask::FutureHasBeenAlreadyMoved();
            }
            state_->moved = true;
            return my_std::Move(state_->value);
        }
        else {
            return static_cast<T>(state_->value);
        }
    }

    std::shared_ptr<TaskBase> GetProducer() const {
        return producer_;
    }
};


class TTaskScheduler;

template<typename Func, typename... Args>
class TTask;


template<typename Func, typename... Args>
class TTaskImpl : public TaskBase {
private:
    TTaskScheduler* scheduler_ = nullptr;

    using DecayedFunc = my_std::decay_t<Func>;
    DecayedFunc func_;

    std::vector<TaskBase*> addicted_to_;

    my_std::Tuple<Args...> args_;

public:
    using ResultType = decltype(my_std::Invoke(my_std::Declval<DecayedFunc&>(), my_std::Declval<unwrap_future_t<Args>>()...));

private:
    std::shared_ptr<TState<ResultType>> state_;

    template<typename Arg>
    static decltype(auto) Materialize(Arg& arg) {
        using BareArg = my_std::remove_cvref_t<Arg>;
        if constexpr (is_future_v<BareArg>) {
            return arg.get();
        }
        else {
            return my_std::Forward<Arg>(arg);
        }
    }

    template<my_std::size_t... I>
    void ExecuteImpl(my_std::IndexSequence<I...>) {
        if constexpr (my_std::is_void_v<ResultType>) {
            my_std::Invoke(func_, Materialize(my_std::Get<I>(args_))...);
        }
        else {
            state_->value = my_std::Invoke(func_, Materialize(my_std::Get<I>(args_))...);
        }
    }

    void Execute() override {
        if (state_->ready) {
            return;
        }
        ExecuteImpl(my_std::make_index_sequence<sizeof...(Args)>{});
        state_->ready = true;
    }

public:
    template<typename F2, typename... A2>
    TTaskImpl(TTaskScheduler* scheduler, F2&& func, A2&&... args)
        : scheduler_(scheduler)
        , func_(my_std::Forward<F2>(func))
        , args_(my_std::Forward<A2>(args)...)
        , state_(std::make_shared<TState<ResultType>>())
    {}

    template<typename T = ResultType>
    T getResultSync();

    template<typename T = ResultType>
    TFuture<T> getFutureResult() {
        using CleanT = my_std::remove_cvref_t<T>;
        static_assert(my_std::is_same_v<CleanT, ResultType>, "Requested value is not like task res type");
        return TFuture<T>(state_, this->shared_from_this());
    }

    void addAddiction(TaskBase* task) {
        addicted_to_.push_back(task);
    }

    template<typename F>
    auto apply(F&& f);

    const std::vector<TaskBase*>& GetAddiction() const override {
        return addicted_to_;
    }

    void ClearSchedulerPointer() override {
        scheduler_ = nullptr;
    }
};


template<typename Func, typename... Args>
class TTask {
public:
    using ResultType = typename TTaskImpl<Func, Args...>::ResultType;

    explicit TTask(std::shared_ptr<TTaskImpl<Func, Args...>> impl) : impl_(my_std::Move(impl)) {}

    template<typename T = ResultType>
    T getResultSync() {
        return impl_->template getResultSync<T>();
    }

    template<typename T = ResultType>
    TFuture<T> getFutureResult() {
        return impl_->template getFutureResult<T>();
    }

    template<typename G>
    auto apply(G&& g) {
        return impl_->apply(my_std::Forward<G>(g));
    }

    template<typename Other>
    void addAddiction(const Other& other) {
        impl_->addAddiction(other.GetTaskBase());
    }

    TaskBase* GetTaskBase() const {
        return impl_.get();
    }

private:
    std::shared_ptr<TTaskImpl<Func, Args...>> impl_;
};


class TTaskScheduler {
private:
    std::vector<std::shared_ptr<TaskBase>> tasks_;

public:
    ~TTaskScheduler() {
        for (auto& task : tasks_) {
            task->ClearSchedulerPointer();
        }
    }

private:
    void DfsExecute(TaskBase* task, std::unordered_map<const TaskBase*, EState>& states) {
        EState& s = states[task];
        if (s == EState::Done) {
            return;
        }
        if (s == EState::Running) {
            throw ttask::CycleInScheduler();
        }
        s = EState::Running;
        for (TaskBase* addiction : task->GetAddiction()) {
            DfsExecute(addiction, states);
        }
        task->Execute();
        s = EState::Done;
    }

public:

    void ExecuteTasksAddicted(TaskBase* task) {
        std::unordered_map<const TaskBase*, EState> states;
        DfsExecute(task, states);
    }

    template<typename Func, typename... Args>
    auto add(Func&& func, Args&&... args) {
        using ImplT = TTaskImpl<my_std::decay_t<Func>, my_std::decay_t<Args>...>;

        std::vector<TaskBase*> addiction; 
        auto AddOneTask = [&](auto&& arg) {
            using Arg = my_std::remove_cvref_t<decltype(arg)>;
            if constexpr (is_future_v<Arg>) {
                if (auto p = arg.GetProducer()) {
                    addiction.push_back(p.get());
                }
            }
        };

        (AddOneTask(args), ...);

        auto impl = std::make_shared<ImplT>(this, my_std::Forward<Func>(func), my_std::Forward<Args>(args)...);

        for (auto* a : addiction) {
            impl->addAddiction(a);
        }

        tasks_.push_back(impl);
        return TTask<my_std::decay_t<Func>, my_std::decay_t<Args>...>{impl};
    }

    void executeAll() {
        std::unordered_map<const TaskBase*, EState> states;
        for (auto& task : tasks_) {
            DfsExecute(task.get(), states);
        }
    }
};

template<typename Func, typename... Args>
template<typename F>
auto TTaskImpl<Func, Args...>::apply(F&& f) {
    static_assert(!my_std::is_void_v<ResultType>, "apply cant be used on tasks that returning void");
    static_assert(my_std::is_invocable_v<F&, ResultType>,
                  "apply expects callable taking exactly one argument compatible with task result");
    if (!scheduler_) {
        throw ttask::EmptyScheduler();
    }
    return scheduler_->add(my_std::Forward<F>(f), getFutureResult<ResultType>());
}

template<typename Func, typename... Args>
template<typename T>
T TTaskImpl<Func, Args...>::getResultSync() {
    using CleanT = my_std::remove_cvref_t<T>;
    static_assert(my_std::is_same_v<CleanT, ResultType>, "Requested value is not like task res type");
    if (!state_->ready) {
        if (!scheduler_) {
            throw ttask::EmptyScheduler();
        }
        scheduler_->ExecuteTasksAddicted(this);
    }
    if constexpr (my_std::is_void_v<T>) {
        return;
    }
    else if constexpr (my_std::is_lvalue_ref_v<T>) {
        return static_cast<T>(state_->value);
    }
    else {
        if (state_->moved) {
            throw ttask::FutureHasBeenAlreadyMoved();
        }
        state_->moved = true;
        return my_std::Move(state_->value);
    }
}