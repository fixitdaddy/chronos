#pragma once

#include <memory>
#include <string>

#include "chronos/persistence/repository.hpp"

namespace chronos::worker::execution {

class ClaimManager {
 public:
  ClaimManager(
      std::string worker_id,
      std::shared_ptr<persistence::IExecutionRepository> execution_repository);

  bool TryClaimExecution(const std::string& execution_id);

 private:
  std::string worker_id_;
  std::shared_ptr<persistence::IExecutionRepository> execution_repository_;
};

}  // namespace chronos::worker::execution
