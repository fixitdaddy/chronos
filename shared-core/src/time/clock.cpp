#include "chronos/time/clock.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace chronos::time {

Timestamp UtcNow() {
  return std::chrono::system_clock::now();
}

std::string ToIso8601(Timestamp timestamp) {
  const auto as_time_t = std::chrono::system_clock::to_time_t(timestamp);

  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &as_time_t);
#else
  gmtime_r(&as_time_t, &tm);
#endif

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

}  // namespace chronos::time
