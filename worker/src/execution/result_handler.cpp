#include "chronos/worker/execution/result_handler.hpp"

#include <optional>
#include <string>
#include <utility>

namespace chronos::worker::execution {

ResultHandler::ResultHandler(std::shared_ptr<persistence::IExecutionRepository> execution_repository)
    : execution_repository_(std::move(execution_repository)) {}

bool ResultHandler::ApplyResult(
    const std::string& execution_id,
    const task::TaskResult& result,
    const std::string& worker_id,
    int attempt_number,
    int max_attempts) {
  switch (result.kind) {
    case task::TaskResultKind::kSuccess:
      return execution_repository_->TransitionExecutionState(
          execution_id,
          domain::ExecutionState::kRunning,
          domain::ExecutionState::kSucceeded,
          std::nullopt,
          std::nullopt,
          worker_id);

    case task::TaskResultKind::kRetryableFailure: {
      if (attempt_number >= max_attempts) {
        return execution_repository_->TransitionExecutionState(
            execution_id,
            domain::ExecutionState::kRunning,
            domain::ExecutionState::kDeadLetter,
            result.error_code,
            result.error_message,
            worker_id);
      }
      return execution_repository_->TransitionExecutionState(
          execution_id,
          domain::ExecutionState::kRunning,
          domain::ExecutionState::kRetryPending,
          result.error_code,
          result.error_message,
          worker_id);
    }

    case task::TaskResultKind::kPermanentFailure:
      return execution_repository_->TransitionExecutionState(
          execution_id,
          domain::ExecutionState::kRunning,
          domain::ExecutionState::kFailed,
          result.error_code.empty() ? std::optional<std::string>("PERMANENT_FAILURE")
                                    : std::optional<std::string>(result.error_code),
          result.error_message,
          worker_id);

    case task::TaskResultKind::kTimeout:
      return execution_repository_->TransitionExecutionState(
          execution_id,
          domain::ExecutionState::kRunning,
          domain::ExecutionState::kRetryPending,
          std::optional<std::string>("TIMEOUT"),
          result.error_message,
          worker_id);

    case task::TaskResultKind::kAbandoned:
      return execution_repository_->TransitionExecutionState(
          execution_id,
          domain::ExecutionState::kRunning,
          domain::ExecutionState::kRetryPending,
          std::optional<std::string>("ABANDONED"),
          result.error_message,
          worker_id);
  }

  return false;
}

}  // namespace chronos::worker::execution
