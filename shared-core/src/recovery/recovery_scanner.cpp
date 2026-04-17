#include "chronos/recovery/recovery_scanner.hpp"

#include "chronos/messaging/message_codec.hpp"
#include "chronos/time/clock.hpp"

namespace chronos::recovery {

RecoveryScanner::RecoveryScanner(
    Config config,
    std::shared_ptr<persistence::IExecutionRepository> execution_repository,
    std::shared_ptr<chronos::messaging::IQueueBroker> broker)
    : config_(std::move(config)),
      execution_repository_(std::move(execution_repository)),
      broker_(std::move(broker)) {}

std::vector<RecoveryScanner::Finding> RecoveryScanner::ScanAndRepair(
    const std::vector<std::string>& candidate_execution_ids,
    const std::string& trace_id) {
  std::vector<Finding> findings;
  const auto now = time::UtcNow();

  for (const auto& id : candidate_execution_ids) {
    const auto execution = execution_repository_->GetExecutionById(id);
    if (!execution.has_value()) {
      continue;
    }

    if (execution->state == domain::ExecutionState::kRunning) {
      if (execution->last_heartbeat_at.has_value()) {
        const auto silent_for = now - execution->last_heartbeat_at.value();
        if (silent_for > config_.missing_heartbeat_after) {
          execution_repository_->TransitionExecutionState(
              execution->execution_id,
              domain::ExecutionState::kRunning,
              domain::ExecutionState::kRetryPending,
              std::optional<std::string>("WORKER_LOST"),
              std::optional<std::string>("heartbeat expired"),
              std::nullopt);
          findings.push_back({execution->execution_id, "lost_worker_heartbeat", "to_retry_pending"});
          continue;
        }
      }

      if (execution->started_at.has_value()) {
        const auto running_for = now - execution->started_at.value();
        if (running_for > config_.stale_running_after) {
          execution_repository_->TransitionExecutionState(
              execution->execution_id,
              domain::ExecutionState::kRunning,
              domain::ExecutionState::kRetryPending,
              std::optional<std::string>("STALE_RUNNING"),
              std::optional<std::string>("running too long"),
              std::nullopt);
          findings.push_back({execution->execution_id, "stale_running", "to_retry_pending"});
        }
      }
      continue;
    }

    if (execution->state == domain::ExecutionState::kDispatched) {
      if (execution->dispatched_at.has_value()) {
        const auto stuck_for = now - execution->dispatched_at.value();
        if (stuck_for > config_.stuck_dispatched_after) {
          chronos::messaging::ExecutionDispatchMessage message;
          message.trace_id = trace_id;
          message.job_id = execution->job_id;
          message.execution_id = execution->execution_id;
          message.attempt = execution->attempt_count + 1;
          message.scheduled_at = execution->scheduled_at;
          message.idempotency_key = execution->execution_id;
          message.payload_json = execution->result_summary_json.value_or("{}");

          broker_->Publish(
              chronos::messaging::kRetryQueue,
              chronos::messaging::EncodeDispatchMessage(message),
              true);

          findings.push_back({execution->execution_id, "stuck_dispatched", "republished_to_retry_queue"});
        }
      }
      continue;
    }

    if (execution->state == domain::ExecutionState::kRetryPending) {
      chronos::messaging::ExecutionDispatchMessage message;
      message.trace_id = trace_id;
      message.job_id = execution->job_id;
      message.execution_id = execution->execution_id;
      message.attempt = execution->attempt_count + 1;
      message.scheduled_at = execution->scheduled_at;
      message.idempotency_key = execution->execution_id;
      message.payload_json = execution->result_summary_json.value_or("{}");

      broker_->Publish(
          chronos::messaging::kRetryQueue,
          chronos::messaging::EncodeDispatchMessage(message),
          true);

      findings.push_back({execution->execution_id, "expired_retry", "republished_to_retry_queue"});
    }
  }

  return findings;
}

}  // namespace chronos::recovery
