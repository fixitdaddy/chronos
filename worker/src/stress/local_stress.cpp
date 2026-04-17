#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "chronos/persistence/in_memory/in_memory_repositories.hpp"
#include "chronos/time/clock.hpp"
#include "chronos/worker/runtime/worker_runtime.hpp"
#include "chronos/worker/task/task_registry.hpp"

int main() {
  using namespace chronos;

  constexpr int kTasks = 500;

  auto audit = std::make_shared<persistence::in_memory::InMemoryAuditRepository>();
  auto executions = std::make_shared<persistence::in_memory::InMemoryExecutionRepository>(audit);
  auto registry = std::make_shared<worker::task::TaskRegistry>();

  std::atomic<int> done{0};

  registry->Register("stress.noop", [&done](const worker::task::TaskInput&,
                                              const worker::task::CancellationToken&) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    done.fetch_add(1);
    worker::task::TaskResult result;
    result.kind = worker::task::TaskResultKind::kSuccess;
    return result;
  });

  worker::runtime::WorkerRuntime::Config config;
  config.worker_id = "worker-stress-1";
  config.concurrency = 16;
  config.default_task_timeout = std::chrono::milliseconds(1000);

  worker::runtime::WorkerRuntime runtime(config, executions, registry);

  for (int i = 0; i < kTasks; ++i) {
    domain::JobExecution execution;
    execution.execution_id = "stress-exec-" + std::to_string(i);
    execution.job_id = "stress-job";
    execution.execution_number = i + 1;
    execution.scheduled_at = time::UtcNow();
    execution.state = domain::ExecutionState::kDispatched;
    execution.max_attempts = 3;
    execution.idempotency_key = std::string("idem-stress-") + std::to_string(i);
    execution.created_at = time::UtcNow();
    execution.updated_at = execution.created_at;
    executions->CreateExecution(execution);

    runtime.SubmitExecution(
        execution.execution_id,
        "stress.noop",
        "{}",
        "idem-stress-" + std::to_string(i),
        3);
  }

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
  while (done.load() < kTasks && std::chrono::steady_clock::now() < deadline) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  runtime.Shutdown();

  std::cout << "stress_done=" << done.load() << " expected=" << kTasks << std::endl;
  return done.load() == kTasks ? 0 : 1;
}
