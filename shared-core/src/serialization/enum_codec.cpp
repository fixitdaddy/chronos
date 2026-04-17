#include "chronos/serialization/enum_codec.hpp"

#include <unordered_map>

namespace chronos::serialization {
namespace {

template <typename T>
using Optional = std::optional<T>;

}  // namespace

std::string ToString(domain::JobState value) {
  switch (value) {
    case domain::JobState::kActive:
      return "ACTIVE";
    case domain::JobState::kPaused:
      return "PAUSED";
    case domain::JobState::kCompleted:
      return "COMPLETED";
    case domain::JobState::kFailed:
      return "FAILED";
    case domain::JobState::kDeleted:
      return "DELETED";
  }
  return "UNKNOWN";
}

std::string ToString(domain::ScheduleType value) {
  switch (value) {
    case domain::ScheduleType::kOneTime:
      return "ONE_TIME";
    case domain::ScheduleType::kCron:
      return "CRON";
  }
  return "UNKNOWN";
}

std::string ToString(domain::ExecutionState value) {
  switch (value) {
    case domain::ExecutionState::kPending:
      return "PENDING";
    case domain::ExecutionState::kDispatched:
      return "DISPATCHED";
    case domain::ExecutionState::kRunning:
      return "RUNNING";
    case domain::ExecutionState::kSucceeded:
      return "SUCCEEDED";
    case domain::ExecutionState::kFailed:
      return "FAILED";
    case domain::ExecutionState::kRetryPending:
      return "RETRY_PENDING";
    case domain::ExecutionState::kDeadLetter:
      return "DEAD_LETTER";
  }
  return "UNKNOWN";
}

std::string ToString(domain::AttemptState value) {
  switch (value) {
    case domain::AttemptState::kCreated:
      return "CREATED";
    case domain::AttemptState::kRunning:
      return "RUNNING";
    case domain::AttemptState::kSucceeded:
      return "SUCCEEDED";
    case domain::AttemptState::kFailed:
      return "FAILED";
    case domain::AttemptState::kTimedOut:
      return "TIMED_OUT";
    case domain::AttemptState::kCancelled:
      return "CANCELLED";
  }
  return "UNKNOWN";
}

std::optional<domain::JobState> ParseJobState(const std::string& value) {
  static const std::unordered_map<std::string, domain::JobState> kMap{
      {"ACTIVE", domain::JobState::kActive},
      {"PAUSED", domain::JobState::kPaused},
      {"COMPLETED", domain::JobState::kCompleted},
      {"FAILED", domain::JobState::kFailed},
      {"DELETED", domain::JobState::kDeleted},
  };
  const auto it = kMap.find(value);
  if (it == kMap.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<domain::ScheduleType> ParseScheduleType(const std::string& value) {
  static const std::unordered_map<std::string, domain::ScheduleType> kMap{
      {"ONE_TIME", domain::ScheduleType::kOneTime},
      {"CRON", domain::ScheduleType::kCron},
  };
  const auto it = kMap.find(value);
  if (it == kMap.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<domain::ExecutionState> ParseExecutionState(const std::string& value) {
  static const std::unordered_map<std::string, domain::ExecutionState> kMap{
      {"PENDING", domain::ExecutionState::kPending},
      {"DISPATCHED", domain::ExecutionState::kDispatched},
      {"RUNNING", domain::ExecutionState::kRunning},
      {"SUCCEEDED", domain::ExecutionState::kSucceeded},
      {"FAILED", domain::ExecutionState::kFailed},
      {"RETRY_PENDING", domain::ExecutionState::kRetryPending},
      {"DEAD_LETTER", domain::ExecutionState::kDeadLetter},
  };
  const auto it = kMap.find(value);
  if (it == kMap.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<domain::AttemptState> ParseAttemptState(const std::string& value) {
  static const std::unordered_map<std::string, domain::AttemptState> kMap{
      {"CREATED", domain::AttemptState::kCreated},
      {"RUNNING", domain::AttemptState::kRunning},
      {"SUCCEEDED", domain::AttemptState::kSucceeded},
      {"FAILED", domain::AttemptState::kFailed},
      {"TIMED_OUT", domain::AttemptState::kTimedOut},
      {"CANCELLED", domain::AttemptState::kCancelled},
  };
  const auto it = kMap.find(value);
  if (it == kMap.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace chronos::serialization
