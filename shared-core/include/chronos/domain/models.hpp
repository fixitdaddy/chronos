#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace chronos::domain {

using Timestamp = std::chrono::system_clock::time_point;

enum class JobState {
  kActive,
  kPaused,
  kCompleted,
  kFailed,
  kDeleted,
};

enum class ScheduleType {
  kOneTime,
  kCron,
};

enum class ExecutionState {
  kPending,
  kDispatched,
  kRunning,
  kSucceeded,
  kFailed,
  kRetryPending,
  kDeadLetter,
};

enum class AttemptState {
  kCreated,
  kRunning,
  kSucceeded,
  kFailed,
  kTimedOut,
  kCancelled,
};

struct RetryPolicy {
  std::int32_t max_attempts{1};
  std::string backoff_strategy{"FIXED"};
  std::int32_t initial_delay_seconds{0};
  std::int32_t max_delay_seconds{0};
  double backoff_multiplier{1.0};
  std::string retryable_error_codes_csv{"NETWORK_ERROR,RATE_LIMITED,TIMEOUT,WORKER_LOST,DEPENDENCY_FAILURE"};
};

struct Job {
  std::string job_id;
  std::string name;
  std::string task_type;
  std::string payload_json;
  std::int32_t priority{0};
  std::string queue_name{"main_queue"};
  std::int32_t max_execution_time_seconds{300};
  std::optional<std::string> idempotency_key;
  JobState state{JobState::kActive};
  Timestamp created_at{};
  Timestamp updated_at{};
};

struct JobSchedule {
  std::string schedule_id;
  std::string job_id;
  ScheduleType schedule_type{ScheduleType::kOneTime};
  std::optional<std::string> cron_expression;
  std::optional<Timestamp> run_at;
  std::string timezone{"UTC"};
  std::optional<Timestamp> start_at;
  std::optional<Timestamp> end_at;
  std::optional<Timestamp> next_run_at;
  std::optional<Timestamp> last_run_at;
  std::string misfire_policy{"FIRE_ONCE"};
  bool active{true};
  Timestamp created_at{};
  Timestamp updated_at{};
};

struct JobExecution {
  std::string execution_id;
  std::string job_id;
  std::optional<std::string> schedule_id;
  std::int64_t execution_number{0};
  Timestamp scheduled_at{};
  std::optional<Timestamp> dispatched_at;
  std::optional<Timestamp> started_at;
  std::optional<Timestamp> finished_at;
  ExecutionState state{ExecutionState::kPending};
  std::int32_t attempt_count{0};
  std::int32_t max_attempts{1};
  std::optional<std::string> idempotency_key;
  std::optional<std::string> current_worker_id;
  std::optional<Timestamp> last_heartbeat_at;
  std::optional<std::string> result_summary_json;
  std::optional<std::string> last_error_code;
  std::optional<std::string> last_error_message;
  std::int32_t poison_count{0};
  std::optional<Timestamp> quarantined_at;
  Timestamp created_at{};
  Timestamp updated_at{};
};

struct JobAttempt {
  std::string attempt_id;
  std::string execution_id;
  std::int32_t attempt_number{1};
  AttemptState state{AttemptState::kCreated};
  std::optional<std::string> worker_id;
  std::optional<Timestamp> started_at;
  std::optional<Timestamp> finished_at;
  std::optional<std::string> error_code;
  std::optional<std::string> error_message;
  Timestamp created_at{};
  Timestamp updated_at{};
};

struct WorkerHeartbeat {
  std::string heartbeat_id;
  std::string worker_id;
  std::string execution_id;
  std::int32_t attempt_number{1};
  Timestamp heartbeat_at{};
  std::optional<std::string> details_json;
};

struct OutboxEvent {
  std::string event_id;
  std::string aggregate_type;
  std::string aggregate_id;
  std::string event_type;
  std::string payload_json;
  Timestamp created_at{};
  std::optional<Timestamp> published_at;
  std::int32_t publish_attempts{0};
};

struct ExecutionTransition {
  ExecutionState from;
  ExecutionState to;
  std::optional<std::string> error_code;
  std::optional<std::string> error_message;
};

}  // namespace chronos::domain
