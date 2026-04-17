#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "chronos/persistence/repository.hpp"
#include "chronos/worker/execution/claim_manager.hpp"
#include "chronos/worker/execution/heartbeat_manager.hpp"
#include "chronos/worker/execution/result_handler.hpp"
#include "chronos/worker/runtime/thread_pool.hpp"
#include "chronos/worker/task/task_registry.hpp"

namespace chronos::worker::runtime {

class WorkerRuntime {
 public:
  struct Config {
    std::string worker_id{"worker-local-1"};
    std::size_t concurrency{8};
    std::chrono::milliseconds heartbeat_interval{1000};
    std::chrono::milliseconds idle_poll_interval{200};
    std::chrono::milliseconds default_task_timeout{5000};
    bool drain_on_shutdown{true};
  };

  WorkerRuntime(
      Config config,
      std::shared_ptr<persistence::IExecutionRepository> execution_repository,
      std::shared_ptr<task::TaskRegistry> task_registry);

  bool SubmitExecution(
      const std::string& execution_id,
      const std::string& task_type,
      const std::string& payload_json,
      const std::string& idempotency_key,
      int max_attempts);

  void Shutdown();

 private:
  Config config_;
  std::shared_ptr<persistence::IExecutionRepository> execution_repository_;
  std::shared_ptr<task::TaskRegistry> task_registry_;
  ThreadPool pool_;
  execution::ClaimManager claim_manager_;
  execution::ResultHandler result_handler_;
  std::atomic<bool> stopping_{false};
};

}  // namespace chronos::worker::runtime
