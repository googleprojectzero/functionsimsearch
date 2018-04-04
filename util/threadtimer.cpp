#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <thread>
#include "util/threadtimer.hpp"

namespace profile {

#ifdef PROFILE
static thread_local std::chrono::time_point<
  std::chrono::high_resolution_clock> timepoint;

void ResetClock() {
  timepoint = std::chrono::high_resolution_clock::now();
}

void ClockCheckpoint(const char* format, ...) {
  va_list args;
  auto microseconds = std::chrono::duration_cast<
    std::chrono::microseconds>(std::chrono::high_resolution_clock::now() -
      timepoint);
  printf("[Thread %d Profiling: %ld microseconds] ",
    std::this_thread::get_id(), microseconds);
  va_start(args, format);
  vprintf(format, args);
  timepoint = std::chrono::high_resolution_clock::now();
}
#else
void ResetClock() {};
void ClockCheckpoint(const char* format, ...) {};
#endif

} // namespace profile
