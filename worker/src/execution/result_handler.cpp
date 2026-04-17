#include "chronos/worker/execution/result_handler.hpp"

#include <optional>
#include <string>
#include <utility>

#include "chronos/domain/models.hpp"
#include "chronos/retry/backoff.hpp"

namespace chronos::worker::execution {

ResultHandler::ResultHandler(std::shared_ptr<persistence::IExecutionRepository> execution_repository)
    : execution_repository_(std::move(execution_repository)) {}

bool ResultHandler::ApplyResult(
    const std::string& execution_id,
    const task::TaskResult& result,
    const std::string& worker_id,
    int attempt_number,
    int max_attempts) {
  domain::RetryPolicy policy;
  policy.max_attempts = max_attempts;
  policy.backoff_strategy = "EXPONENTIAL";
  policy.initial_delay_seconds = 1;
  policy.max_delay_seconds = 60;
  policy.backoff_multiplier = 2.0;
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
      if (retry::ShouldMoveToDeadLetter(attempt_number, policy, result.error_code)) {
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

    case task::TaskResultKind::kTimeout: {
      if (retry::ShouldMoveToDeadLetter(attempt_number, policy, "TIMEOUT")) {
        return execution_repository_->TransitionExecutionState(
            execution_id,
            domain::ExecutionState::kRunning,
            domain::ExecutionState::kDeadLetter,
            std::optional<std::string>("TIMEOUT"),
            result.error_message,
            worker_id);
      }
      return execution_repository_->TransitionExecutionState(
          execution_id,
          domain::ExecutionState::kRunning,
          domain::ExecutionState::kRetryPending,
          std::optional<std::string>("TIMEOUT"),
          result.error_message,
          worker_id);
    }

    case task::TaskResultKind::kAbandoned: {
      if (retry::ShouldMoveToDeadLetter(attempt_number, policy, "ABANDONED")) {
        return execution_repository_->TransitionExecutionState(
            execution_id,
            domain::ExecutionState::kRunning,
            domain::ExecutionState::kDeadLetter,
            std::optional<std::string>("ABANDONED"),
            result.error_message,
            worker_id);
      }
      return execution_repository_->TransitionExecutionState(
          execution_id,
          domain::ExecutionState::kRunning,
          domain::ExecutionState::kRetryPending,
          std::optional<std::string>("ABANDONED"),
          result.error_message,
          worker_id);
    }
  }

  return false;
}

}  // namespace chronos::worker::execution
