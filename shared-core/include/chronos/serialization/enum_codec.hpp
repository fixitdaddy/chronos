#pragma once

#include <optional>
#include <string>

#include "chronos/domain/models.hpp"

namespace chronos::serialization {

std::string ToString(domain::JobState value);
std::string ToString(domain::ScheduleType value);
std::string ToString(domain::ExecutionState value);
std::string ToString(domain::AttemptState value);

std::optional<domain::JobState> ParseJobState(const std::string& value);
std::optional<domain::ScheduleType> ParseScheduleType(const std::string& value);
std::optional<domain::ExecutionState> ParseExecutionState(const std::string& value);
std::optional<domain::AttemptState> ParseAttemptState(const std::string& value);

}  // namespace chronos::serialization
