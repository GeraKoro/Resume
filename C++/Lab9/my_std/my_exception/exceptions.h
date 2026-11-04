#pragma once

namespace ttask {

struct FutureHasBeenAlreadyMoved {};
struct CycleInScheduler {};
struct FutureIsNotReady {};
struct EmptyScheduler {};

} // namespace ttask