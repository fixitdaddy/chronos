#pragma once

#include <cstdint>
#include <string>

#include "chronos/domain/models.hpp"

namespace chronos::retry {

std::int32_t ComputeDelaySeconds(
    const domain::RetryPolicy& policy,
    std::int32_t attempt_number);

bool ShouldMoveToDeadLetter(
    std::int32_t attempt_number,
    const domain::RetryPolicy& policy,
    const std::string& error_code);

}  // namespace chronos::retry
