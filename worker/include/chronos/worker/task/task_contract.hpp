#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>

namespace chronos::worker::task {

struct TaskInput {
  std::string execution_id;
  std::string task_type;
  std::string payload_json;
  std::string idempotency_key;
  std::chrono::milliseconds timeout{30000};
};

struct CancellationToken {
  bool requested{false};
};

enum class TaskResultKind {
  kSuccess,
  kRetryableFailure,
  kPermanentFailure,
  kTimeout,
  kAbandoned,
};

struct TaskResult {
  TaskResultKind kind{TaskResultKind::kSuccess};
  std::string error_code;
  std::string error_message;
  std::string metadata_json{"{}"};
};

using TaskHandler = std::function<TaskResult(const TaskInput&, const CancellationToken&)>;

std::string ToString(TaskResultKind kind);

}  // namespace chronos::worker::task
