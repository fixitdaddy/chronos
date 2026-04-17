#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "chronos/messaging/queue_broker.hpp"
#include "chronos/persistence/repository.hpp"

namespace chronos::recovery {

class RecoveryScanner {
 public:
  struct Finding {
    std::string execution_id;
    std::string category;
    std::string action;
  };

  struct Config {
    std::chrono::seconds stale_running_after{60};
    std::chrono::seconds missing_heartbeat_after{30};
    std::chrono::seconds stuck_dispatched_after{45};
  };

  RecoveryScanner(
      Config config,
      std::shared_ptr<persistence::IExecutionRepository> execution_repository,
      std::shared_ptr<chronos::messaging::IQueueBroker> broker);

  std::vector<Finding> ScanAndRepair(
      const std::vector<std::string>& candidate_execution_ids,
      const std::string& trace_id);

 private:
  Config config_;
  std::shared_ptr<persistence::IExecutionRepository> execution_repository_;
  std::shared_ptr<chronos::messaging::IQueueBroker> broker_;
};

}  // namespace chronos::recovery
