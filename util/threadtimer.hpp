#ifndef THREADTIMER_HPP
#define THREADTIMER_HPP

#include <chrono>

namespace profile {

void ResetClock();
void ClockCheckpoint(const char* format, ...);

} // namespace profile

#endif // THREADTIMER_HPP
