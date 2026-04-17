#include "chronos/worker/runtime/worker_runtime.hpp"

#include <future>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>

#include "chronos/worker/observability/logger.hpp"

namespace chronos::worker::runtime {

WorkerRuntime::WorkerRuntime(
    Config config,
    std::shared_ptr<persistence::IExecutionRepository> execution_repository,
    std::shared_ptr<task::TaskRegistry> task_registry)
    : config_(std::move(config)),
      execution_repository_(std::move(execution_repository)),
      task_registry_(std::move(task_registry)),
      pool_(config_.concurrency),
      claim_manager_(config_.worker_id, execution_repository_),
      result_handler_(execution_repository_) {}

bool WorkerRuntime::SubmitExecution(
    const std::string& execution_id,
    const std::string& task_type,
    const std::string& payload_json,
    const std::string& idempotency_key,
    int max_attempts) {
  if (stopping_.load()) {
    return false;
  }

  if (!claim_manager_.TryClaimExecution(execution_id)) {
    observability::Log("warn", config_.worker_id, "execution_claim_failed", "{\"execution_id\":\"" + execution_id + "\"}");
    return false;
  }

  auto handler = task_registry_->Resolve(task_type);
  if (!handler.has_value()) {
    task::TaskResult result;
    result.kind = task::TaskResultKind::kPermanentFailure;
    result.error_code = "TASK_HANDLER_NOT_FOUND";
    result.error_message = "no handler registered for task_type";
    return result_handler_.ApplyResult(execution_id, result, config_.worker_id, 1, max_attempts);
  }

  try {
    pool_.Submit([this,
                  execution_id,
                  payload_json,
                  idempotency_key,
                  max_attempts,
                  task_type,
                  handler = *handler]() {
      execution::HeartbeatManager heartbeat(
          config_.worker_id,
          execution_repository_,
          config_.heartbeat_interval);
      heartbeat.Start(execution_id, 1);

      task::TaskInput input;
      input.execution_id = execution_id;
      input.task_type = task_type;
      input.payload_json = payload_json;
      input.idempotency_key = idempotency_key;
      input.timeout = config_.default_task_timeout;

      task::CancellationToken token;

      auto future = std::async(std::launch::async, [handler, input, token]() {
        return handler(input, token);
      });

      task::TaskResult result;
      if (future.wait_for(input.timeout) == std::future_status::timeout) {
        result.kind = task::TaskResultKind::kTimeout;
        result.error_code = "TIMEOUT";
        result.error_message = "task timed out";
      } else {
        result = future.get();
      }

      heartbeat.Stop();
      result_handler_.ApplyResult(execution_id, result, config_.worker_id, 1, max_attempts);
    });
  } catch (const std::exception&) {
    return false;
  }

  return true;
}

void WorkerRuntime::Shutdown() {
  stopping_.store(true);
  pool_.Shutdown(config_.drain_on_shutdown);
}

}  // namespace chronos::worker::runtime
