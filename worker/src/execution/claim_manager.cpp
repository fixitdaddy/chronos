#include "chronos/worker/execution/claim_manager.hpp"

#include <optional>
#include <utility>

namespace chronos::worker::execution {

ClaimManager::ClaimManager(
    std::string worker_id,
    std::shared_ptr<persistence::IExecutionRepository> execution_repository)
    : worker_id_(std::move(worker_id)),
      execution_repository_(std::move(execution_repository)) {}

bool ClaimManager::TryClaimExecution(const std::string& execution_id) {
  // Transactionally enforced in repository via expected state check.
  return execution_repository_->TransitionExecutionState(
      execution_id,
      domain::ExecutionState::kDispatched,
      domain::ExecutionState::kRunning,
      std::nullopt,
      std::nullopt,
      worker_id_);
}

}  // namespace chronos::worker::execution
