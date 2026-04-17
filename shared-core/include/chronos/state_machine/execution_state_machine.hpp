#pragma once

#include <optional>
#include <string>

#include "chronos/domain/models.hpp"

namespace chronos::state_machine {

struct TransitionDecision {
  bool allowed{false};
  std::string reason;
};

TransitionDecision ValidateExecutionTransition(
    domain::ExecutionState from,
    domain::ExecutionState to);

std::optional<domain::ExecutionTransition> BuildTransition(
    domain::ExecutionState from,
    domain::ExecutionState to,
    std::optional<std::string> error_code = std::nullopt,
    std::optional<std::string> error_message = std::nullopt);

}  // namespace chronos::state_machine
