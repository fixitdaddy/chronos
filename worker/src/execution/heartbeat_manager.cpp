#include "chronos/worker/execution/heartbeat_manager.hpp"

#include <chrono>
#include <optional>
#include <thread>
#include <utility>

#include "chronos/time/clock.hpp"

namespace chronos::worker::execution {

HeartbeatManager::HeartbeatManager(
    std::string worker_id,
    std::shared_ptr<persistence::IExecutionRepository> execution_repository,
    std::chrono::milliseconds interval)
    : worker_id_(std::move(worker_id)),
      execution_repository_(std::move(execution_repository)),
      interval_(interval) {}

HeartbeatManager::~HeartbeatManager() {
  Stop();
}

void HeartbeatManager::Start(const std::string& execution_id, int attempt_number) {
  Stop();
  execution_id_ = execution_id;
  attempt_number_ = attempt_number;
  running_.store(true);
  thread_ = std::thread([this]() { Loop(); });
}

void HeartbeatManager::Stop() {
  running_.store(false);
  if (thread_.joinable()) {
    thread_.join();
  }
}

void HeartbeatManager::Loop() {
  while (running_.load()) {
    domain::WorkerHeartbeat hb;
    hb.heartbeat_id = worker_id_ + "-" + execution_id_ + "-hb";
    hb.worker_id = worker_id_;
    hb.execution_id = execution_id_;
    hb.attempt_number = attempt_number_;
    hb.heartbeat_at = time::UtcNow();
    hb.details_json = std::string("{\"source\":\"worker-heartbeat\"}");

    execution_repository_->RecordWorkerHeartbeat(hb);
    std::this_thread::sleep_for(interval_);
  }
}

}  // namespace chronos::worker::execution
