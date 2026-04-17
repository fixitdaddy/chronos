#pragma once

#include <memory>
#include <string>

#include "chronos/persistence/repository.hpp"
#include "chronos/worker/task/task_contract.hpp"

namespace chronos::worker::execution {

class ResultHandler {
 public:
  explicit ResultHandler(std::shared_ptr<persistence::IExecutionRepository> execution_repository);

  bool ApplyResult(
      const std::string& execution_id,
      const task::TaskResult& result,
      const std::string& worker_id,
      int attempt_number,
      int max_attempts);

 private:
  std::shared_ptr<persistence::IExecutionRepository> execution_repository_;
};

}  // namespace chronos::worker::execution
