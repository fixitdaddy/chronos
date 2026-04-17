#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "chronos/persistence/repository.hpp"

namespace chronos::worker::execution {

class HeartbeatManager {
 public:
  HeartbeatManager(
      std::string worker_id,
      std::shared_ptr<persistence::IExecutionRepository> execution_repository,
      std::chrono::milliseconds interval);
  ~HeartbeatManager();

  void Start(const std::string& execution_id, int attempt_number);
  void Stop();

 private:
  void Loop();

  std::string worker_id_;
  std::shared_ptr<persistence::IExecutionRepository> execution_repository_;
  std::chrono::milliseconds interval_;
  std::atomic<bool> running_{false};
  std::thread thread_;
  std::string execution_id_;
  int attempt_number_{1};
};

}  // namespace chronos::worker::execution
