#include "chronos/retry/backoff.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace chronos::retry {
namespace {

std::vector<std::string> SplitCsv(const std::string& csv) {
  std::vector<std::string> out;
  std::stringstream ss(csv);
  std::string item;
  while (std::getline(ss, item, ',')) {
    if (!item.empty()) {
      out.push_back(item);
    }
  }
  return out;
}

}  // namespace

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

  // FIXED and fallback behavior.
  return std::max(0, policy.initial_delay_seconds);
}

bool IsRetryableError(
    const domain::RetryPolicy& policy,
    const std::string& error_code) {
  if (error_code.empty()) {
    return true;
  }

  const auto codes = SplitCsv(policy.retryable_error_codes_csv);
  return std::find(codes.begin(), codes.end(), error_code) != codes.end();
}

bool ShouldMoveToDeadLetter(
    std::int32_t attempt_number,
    const domain::RetryPolicy& policy,
    const std::string& error_code) {
  if (attempt_number >= policy.max_attempts) {
    return true;
  }

  if (!IsRetryableError(policy, error_code)) {
    return true;
  }

  if (error_code == "INVALID_PAYLOAD") {
    return true;
  }

  return false;
}

}  // namespace chronos::retry
