#include "chronos/state_machine/execution_state_machine.hpp"

#include <set>
#include <utility>

namespace chronos::state_machine {
namespace {

using State = domain::ExecutionState;

const std::set<std::pair<State, State>> kAllowedTransitions = {
    {State::kPending, State::kDispatched},
    {State::kDispatched, State::kRunning},
    {State::kRunning, State::kSucceeded},
    {State::kRunning, State::kFailed},
    {State::kRunning, State::kRetryPending},
    {State::kRunning, State::kDeadLetter},
    {State::kRetryPending, State::kDispatched},
};

}  // namespace

TransitionDecision ValidateExecutionTransition(
    domain::ExecutionState from,
    domain::ExecutionState to) {
  if (from == to) {
    return {.allowed = false, .reason = "self transition is not allowed"};
  }

  if (kAllowedTransitions.contains({from, to})) {
    return {.allowed = true, .reason = "ok"};
  }

  return {.allowed = false, .reason = "invalid execution state transition"};
}

std::optional<domain::ExecutionTransition> BuildTransition(
    domain::ExecutionState from,
    domain::ExecutionState to,
    std::optional<std::string> error_code,
    std::optional<std::string> error_message) {
  const auto decision = ValidateExecutionTransition(from, to);
  if (!decision.allowed) {
    return std::nullopt;
  }

  return domain::ExecutionTransition{
      .from = from,
      .to = to,
      .error_code = std::move(error_code),
      .error_message = std::move(error_message),
  };
}

}  // namespace chronos::state_machine
