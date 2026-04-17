#pragma once

#include <memory>
#include <string>

#include "chronos/persistence/in_memory/in_memory_repositories.hpp"

namespace chronos::worker::recovery {

class PoisonDetector {
 public:
  struct Config {
    int poison_threshold{3};
  };

  PoisonDetector(
      Config config,
      std::shared_ptr<persistence::in_memory::InMemoryExecutionRepository> execution_repository);

  bool EvaluateAndQuarantine(const std::string& execution_id, const std::string& error_code);

 private:
  Config config_;
  std::shared_ptr<persistence::in_memory::InMemoryExecutionRepository> execution_repository_;
};

}  // namespace chronos::worker::recovery
