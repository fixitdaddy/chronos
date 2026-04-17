#include <cassert>
#include <cstdlib>

#include "chronos/retry/backoff.hpp"

int main() {
  chronos::domain::RetryPolicy policy;
  policy.max_attempts = 5;
  policy.backoff_strategy = "EXPONENTIAL";
  policy.initial_delay_seconds = 10;
  policy.max_delay_seconds = 120;
  policy.backoff_multiplier = 2.0;

  const auto d1 = chronos::retry::ComputeDelaySeconds(policy, 1);
  const auto d2 = chronos::retry::ComputeDelaySeconds(policy, 2);

  assert(d1 == 10);
  assert(d2 == 20);

  assert(!chronos::retry::ShouldMoveToDeadLetter(2, policy, "NETWORK_ERROR"));
  assert(chronos::retry::ShouldMoveToDeadLetter(5, policy, "NETWORK_ERROR"));
  assert(chronos::retry::ShouldMoveToDeadLetter(1, policy, "INVALID_PAYLOAD"));

  return EXIT_SUCCESS;
}
