#include <cassert>
#include <cstdlib>

#include "chronos/state_machine/execution_state_machine.hpp"

int main() {
  using chronos::domain::ExecutionState;
  using chronos::state_machine::ValidateExecutionTransition;

  {
    const auto ok = ValidateExecutionTransition(
        ExecutionState::kPending,
        ExecutionState::kDispatched);
    assert(ok.allowed);
  }

  {
    const auto ok = ValidateExecutionTransition(
        ExecutionState::kRunning,
        ExecutionState::kSucceeded);
    assert(ok.allowed);
  }

  {
    const auto bad = ValidateExecutionTransition(
        ExecutionState::kPending,
        ExecutionState::kRunning);
    assert(!bad.allowed);
  }

  return EXIT_SUCCESS;
}
