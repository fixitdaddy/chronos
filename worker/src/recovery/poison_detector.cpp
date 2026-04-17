#include "chronos/worker/recovery/poison_detector.hpp"

namespace chronos::worker::recovery {

PoisonDetector::PoisonDetector(
    Config config,
    std::shared_ptr<persistence::in_memory::InMemoryExecutionRepository> execution_repository)
    : config_(std::move(config)),
      execution_repository_(std::move(execution_repository)) {}

bool PoisonDetector::EvaluateAndQuarantine(
    const std::string& execution_id,
    const std::string& error_code) {
  (void)error_code;
  const auto execution = execution_repository_->GetExecutionById(execution_id);
  if (!execution.has_value()) {
    return false;
  }

  if (execution->poison_count + 1 >= config_.poison_threshold) {
    return execution_repository_->MarkExecutionQuarantined(execution_id);
  }

  return false;
}

}  // namespace chronos::worker::recovery
