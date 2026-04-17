#include "chronos/retry/backoff.hpp"

#include <algorithm>
#include <cmath>

namespace chronos::retry {

std::int32_t ComputeDelaySeconds(
    const domain::RetryPolicy& policy,
    std::int32_t attempt_number) {
  const auto normalized_attempt = std::max(1, attempt_number);

  if (policy.backoff_strategy == "EXPONENTIAL") {
    const auto exponent = std::max(0, normalized_attempt - 1);
    const auto raw_delay = static_cast<double>(policy.initial_delay_seconds) *
                           std::pow(policy.backoff_multiplier, exponent);
    const auto clamped = std::clamp(
        static_cast<std::int32_t>(raw_delay),
        policy.initial_delay_seconds,
        std::max(policy.initial_delay_seconds, policy.max_delay_seconds));
    return clamped;
  }

  if (policy.backoff_strategy == "LINEAR") {
    const auto raw_delay = policy.initial_delay_seconds * normalized_attempt;
    return std::min(raw_delay, std::max(policy.initial_delay_seconds, policy.max_delay_seconds));
  }

  return std::max(0, policy.initial_delay_seconds);
}

bool ShouldMoveToDeadLetter(
    std::int32_t attempt_number,
    const domain::RetryPolicy& policy,
    const std::string& error_code) {
  if (attempt_number >= policy.max_attempts) {
    return true;
  }

  if (error_code == "INVALID_PAYLOAD") {
    return true;
  }

  return false;
}

}  // namespace chronos::retry
