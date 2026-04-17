#pragma once

#include <chrono>
#include <string>

namespace chronos::time {

using Timestamp = std::chrono::system_clock::time_point;

Timestamp UtcNow();
std::string ToIso8601(Timestamp timestamp);

}  // namespace chronos::time
