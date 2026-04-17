#pragma once

#include <string>

namespace chronos::worker::observability {

void Log(
    const std::string& level,
    const std::string& worker_id,
    const std::string& message,
    const std::string& extra_json = "{}");

}  // namespace chronos::worker::observability
