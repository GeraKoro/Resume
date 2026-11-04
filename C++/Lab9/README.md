# Лабораторная работа 9

Планировщик задач

## Задача

Вашей задачей будет разработать класс, отвечающий за выполнение задач и выставление взаимосвязей между ними (фактически это класс для построения [DAG](https://en.wikipedia.org/wiki/Directed_acyclic_graph)). 

Для 1 и 2 потока также необходимо будет реализовать приложение, рекомендующее места отдыха в зависимости от местоположения и погодных условий.

Часто, чтобы решить какую-либо задачу, требуется выполнить граф вычислений, где узел графа — это задача, а ребро — связь между результатом выполнения одной задачи и параметром для запуска другой. Вам предстоит разработать класс **TTaskScheduler**, решающий подобную задачу.

Такой класс помогает понятно разбить задачу по этапам и построить взаимосвязь между этапами исполнения.

Допустим, существует задача: посчитать средний балл в группах. На вход подаются 2 [csv](https://en.wikipedia.org/wiki/Comma-separated_values) файла: `(isu, gpa)` и `(group, isu)`

Вот так мог бы выглядеть код для решения данной задачи с помощью TTaskScheduler

```cpp

TTaskScheduler scheduler;

auto students = scheduler.add(
    [](const std::string& filePath) {
        std::ifstream file(filePath);
        std::string line;
        std::getline(file, line); // skip header

        std::unordered_map<int, int> gpaByIsu;
        while (std::getline(file, line)) {
            int isu, gpa;
            std::sscanf(line.c_str(), "%d,%d", &isu, &gpa);
            gpaByIsu[isu] = gpa;
        }
        return gpaByIsu;
    },
    "students.csv"
);

auto groups = scheduler.add(
    [](const std::string& filePath) {
        std::ifstream file(filePath);
        std::string line;
        std::getline(file, line); // skip header

        std::unordered_map<std::string, std::vector<int>> groupToIsus;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string group;
            int isu;
            std::getline(ss, group, ',');
            ss >> isu;
            groupToIsus[group].push_back(isu);
        }
        return groupToIsus;
    },
    "groups.csv"
);

// Берём ссылку: таких future result можно получить много раз
auto studentsFuture =
    students.getFutureResult<const std::unordered_map<int, int>&>();
// Берём значение, ожидаем только move, никаких copy, соответственно get можно вызвать единожды
auto groupsFuture =
    groups.getFutureResult<std::unordered_map<std::string, std::vector<int>>>();

auto groupGpa = scheduler.add(
    [](
        const std::unordered_map<int, int>& gpaByIsu,
        std::unordered_map<std::string, std::vector<int>> groupToIsus // ждём, что нам мувнут
    ) {
        std::unordered_map<std::string, int> gpaByGroup;
        for (const auto& [group, isus] : groupToIsus) {
            int sum = 0, cnt = 0;
            for (const int isu : isus) {
                auto it = gpaByIsu.find(isu);
                if (it != gpaByIsu.end()) {
                    sum += it->second;
                    ++cnt;
                }
            }
            if (cnt != 0) {
                gpaByGroup[group] = sum / cnt;
            }
        }
        return gpaByGroup;
    },
    studentsFuture,
    groupsFuture
);

groupGpa.apply(
    [](std::unordered_map<std::string, int>&& gpaByGroup) {
        for (const auto& [group, gpa] : gpaByGroup) {
            std::cout << group << ": " << gpa << "\n";
        }
    }
);

scheduler.executeAll();

// Так делать нельзя, потому что уже мувнули, ожидаем exception
// groupsFuture.get()

// Так можно
const std::unordered_map<int, int>& gpaByIsu = studentsFuture.get();

```

### Публичный интерфейс `TTaskScheduler`

 - **add** &mdash; принимает в качестве аргумента задание и его аргументы. Возвращает объект `TTask`, описывающий добавленную таску.
 - **executeAll** &mdash; выполняет все запланированные задания
 
### Публичный интерфейс `TTask`
 - **getResultSync<T>** &mdash; возвращает результат выполнения задания определённого типа. Вычисляет его, если он ещё не вычислен; при этом не происходит вычисления ненужных заданий
 - **getFutureResult<T>** &mdash; возвращает объект, из которого в будущем можно получить результат задания (фактически ничего не вычисляя), с типом результата T
 
    Если просят T, то ожидается, что объект "переместим".
    Очевидно, что результат future можно "переместить" только единожды, поэтому при последующих обращениях стоит выбрасывать исключение.

    Если просят `cv T&`, то отдаём соответствующую ссылку.
    Также очевидно, что таких ссылок можно получить любое количество.
 - **apply** &mdash; принимает в качестве аргумента задание. Возвращает объект `TTask`. 
 
    Аргументы для задания передаются от предыдущего задания (см. [Требования и ограничения к заданиям](#требования-и-ограничения-к-заданиям))
    
    Также apply может принимать как по rvalue, так и по lvalue ссылке.

### Публичный интерфейс `TFuture`
 - Реализуется на ваше усмотрение; главное, что в зависимости от способа создания этой TFuture (см. выше getFutureResult) выполняется перемещение или берется ссылка.

### Требования и ограничения к заданиям

  - [Callable object](https://en.cppreference.com/w/cpp/named_req/Callable)
  - Количество аргументов &mdash; любое.
  - Задание может быть указателем на метод класса. В таком случае первый аргумент — объект класса, у которого будет вызван метод.
  - `task.apply(func)` может быть применено только тогда, если `func` принимает ровно один аргумент
  
    По желанию можно реализовать `apply` для `func` от любого числа аргументов. Это возможно, например, если `task` возвращает `std::tuple<Ts...>`


## Приложение для рекомендаций (только 1 и 2 поток)

Используя реализованный вами TTaskScheduler, необходимо написать приложение, которое:
1. Узнает ваше местоположение
2. По местоположению:
    - Определит погоду
    - Найдёт все интересные места рядом: музеи, парки, рестораны и т. д. (на ваше усмотрение)
3. Основываясь на погоде выберет интересные места (нелогично рекомендовать гулять по парку, если будет ливень)
4. Выведет краткую сводку про погоду и N мест, куда рекомендуется сходить, и расстояние до них

Можно выбрать любые API; вот те, что уже подобрали для вас:
1. [2ip](https://2ip.ru/free/) &mdash; для определения долготы и широты по IP-адресу
2. [Яндекс.Погода](https://yandex.ru/dev/weather/#start)
3. [Поиск по Яндекс.Картам](https://yandex.ru/maps-api/docs/geosearch-api/index.html)
4. [Матрицы расстояний](https://yandex.ru/maps-api/docs/distancematrix-api/index.html) или [Получение деталей маршрута](https://yandex.ru/maps-api/docs/router-api/index.html)

## Ограничения

 Запрещено использовать стандартную библиотеку, за исключением [контейнеров](https://en.cppreference.com/w/cpp/container) и [умных указателей](https://en.cppreference.com/w/cpp/memory).

## Тесты

Код библиотеки должен быть покрыт тестами. Тесты являются частью задания. Качество написанных тестов и покрытие кода тестами влияют на итоговую оценку.

## NB

1. В данной работе могут быть использованы идеи [Type Erasure](https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Type_Erasure), которые мы разбирали на лекции
2. Мы поговорили с вами про std::forward, подумайте над тем, где это может быть применимо в данной задаче
3. Получаемые расписания не всегда могут быть выполнимы; предлагается подумать, что делать в таких ситуациях
4. Качество рекомендации не является частью оценки. Главное, что от вас ожидается &mdash; применение вашей библиотеки в реальном сценарии.
5. Для упрощения парсинга json обратите внимание на [макросы в библиотеке nlohmann](https://json.nlohmann.me/features/arbitrary_types/#simplify-your-life-with-macros)
6. Также, при желании, некоторые задания можно выполнять [асинхронно](https://www.geeksforgeeks.org/javascript/synchronous-and-asynchronous-programming/) (NB: это не является частью задания).

## ТеорМин

1. Value categories
2. RValue reference
3. Move Semantics
4. Perfect Forwarding
5. Variadic templates

## Deadline

1. 29.04.26 23:59 0.8
2. 06.05.26 23:59 0.65
3. 13.05.26 23:59 0.5

Максимальное количество баллов — 12
