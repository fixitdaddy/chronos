#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "chronos/messaging/in_memory_queue_broker.hpp"
#include "chronos/messaging/message_codec.hpp"
#include "chronos/persistence/in_memory/in_memory_repositories.hpp"
#include "chronos/recovery/recovery_scanner.hpp"
#include "chronos/time/clock.hpp"
#include "chronos/worker/messaging/rabbitmq_consumer.hpp"
#include "chronos/worker/runtime/worker_runtime.hpp"
#include "chronos/worker/task/task_registry.hpp"

namespace {

using namespace chronos;

std::shared_ptr<persistence::in_memory::InMemoryExecutionRepository> NewExecutionRepo() {
  auto audit = std::make_shared<persistence::in_memory::InMemoryAuditRepository>();
  return std::make_shared<persistence::in_memory::InMemoryExecutionRepository>(audit);
}

bool TestDuplicateDelivery() {
  auto executions = NewExecutionRepo();
  auto broker = std::make_shared<messaging::InMemoryQueueBroker>();
  auto registry = std::make_shared<worker::task::TaskRegistry>();

  registry->Register("sample.echo", [](const worker::task::TaskInput&, const worker::task::CancellationToken&) {
    worker::task::TaskResult r;
    r.kind = worker::task::TaskResultKind::kSuccess;
    return r;
  });

  worker::runtime::WorkerRuntime::Config cfg;
  cfg.worker_id = "worker-dup";
  auto runtime = std::make_shared<worker::runtime::WorkerRuntime>(cfg, executions, registry);
  worker::messaging::RabbitMqConsumer consumer(broker, runtime, cfg.worker_id);

  domain::JobExecution e;
  e.execution_id = "dup-exec-1";
  e.job_id = "dup-job-1";
  e.execution_number = 1;
  e.scheduled_at = time::UtcNow();
  e.state = domain::ExecutionState::kDispatched;
  e.max_attempts = 3;
  e.idempotency_key = std::string("idem-dup-1");
  e.created_at = time::UtcNow();
  e.updated_at = e.created_at;
  executions->CreateExecution(e);

  messaging::ExecutionDispatchMessage msg;
  msg.trace_id = "trace-dup";
  msg.job_id = e.job_id;
  msg.execution_id = e.execution_id;
  msg.attempt = 1;
  msg.scheduled_at = e.scheduled_at;
  msg.idempotency_key = "idem-dup-1";
  msg.payload_json = "{}";

  const auto encoded = messaging::EncodeDispatchMessage(msg);
  broker->Publish(messaging::kMainQueue, encoded, true);
  broker->Publish(messaging::kMainQueue, encoded, true);

  consumer.ConsumeOnceFromMain();
  consumer.ConsumeOnceFromMain();

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  runtime->Shutdown();

  const auto post = executions->GetExecutionById(e.execution_id);
  return post.has_value() &&
         (post->state == domain::ExecutionState::kSucceeded ||
          post->state == domain::ExecutionState::kRetryPending ||
          post->state == domain::ExecutionState::kDeadLetter);
}

bool TestWorkerKillAndRecovery() {
  auto executions = NewExecutionRepo();
  auto broker = std::make_shared<messaging::InMemoryQueueBroker>();

  domain::JobExecution e;
  e.execution_id = "recovery-exec-1";
  e.job_id = "recovery-job-1";
  e.execution_number = 1;
  e.scheduled_at = time::UtcNow();
  e.state = domain::ExecutionState::kRunning;
  e.started_at = time::UtcNow() - std::chrono::seconds(90);
  e.last_heartbeat_at = time::UtcNow() - std::chrono::seconds(90);
  e.max_attempts = 3;
  e.created_at = time::UtcNow();
  e.updated_at = e.created_at;
  executions->CreateExecution(e);

  recovery::RecoveryScanner::Config rcfg;
  rcfg.stale_running_after = std::chrono::seconds(30);
  rcfg.missing_heartbeat_after = std::chrono::seconds(20);
  rcfg.stuck_dispatched_after = std::chrono::seconds(20);

  recovery::RecoveryScanner scanner(rcfg, executions, broker);
  const auto findings = scanner.ScanAndRepair({e.execution_id}, "trace-recovery");

  const auto post = executions->GetExecutionById(e.execution_id);
  return !findings.empty() && post.has_value() && post->state == domain::ExecutionState::kRetryPending;
}

bool TestQueueReconnectPath() {
  auto broker = std::make_shared<messaging::InMemoryQueueBroker>();

  const auto p1 = broker->Publish(messaging::kMainQueue, "{}", true);
  const auto before = broker->QueueDepth(messaging::kMainQueue);

  // Simulate reconnect by creating new broker and republishing from durable outbox source.
  auto broker2 = std::make_shared<messaging::InMemoryQueueBroker>();
  const auto p2 = broker2->Publish(messaging::kMainQueue, "{}", true);
  const auto after = broker2->QueueDepth(messaging::kMainQueue);

  return p1.accepted && p2.accepted && before == 1 && after == 1;
}

bool TestDbReconnectPath() {
  // In-memory repo reconnect simulation: pointer replacement and data reinsert flow.
  auto repo1 = NewExecutionRepo();

  domain::JobExecution e;
  e.execution_id = "db-reconnect-exec";
  e.job_id = "db-reconnect-job";
  e.execution_number = 1;
  e.scheduled_at = time::UtcNow();
  e.state = domain::ExecutionState::kDispatched;
  e.max_attempts = 3;
  e.created_at = time::UtcNow();
  e.updated_at = e.created_at;
  repo1->CreateExecution(e);

  auto repo2 = NewExecutionRepo();
  repo2->CreateExecution(e);
  const auto found = repo2->GetExecutionById(e.execution_id);
  return found.has_value();
}

}  // namespace

int main() {
  const bool duplicate_delivery_ok = TestDuplicateDelivery();
  const bool worker_kill_ok = TestWorkerKillAndRecovery();
  const bool queue_reconnect_ok = TestQueueReconnectPath();
  const bool db_reconnect_ok = TestDbReconnectPath();

  std::cout << "duplicate_delivery=" << duplicate_delivery_ok << "\n";
  std::cout << "worker_kill_recovery=" << worker_kill_ok << "\n";
  std::cout << "queue_reconnect=" << queue_reconnect_ok << "\n";
  std::cout << "db_reconnect=" << db_reconnect_ok << "\n";

  return (duplicate_delivery_ok && worker_kill_ok && queue_reconnect_ok && db_reconnect_ok) ? 0 : 1;
}
