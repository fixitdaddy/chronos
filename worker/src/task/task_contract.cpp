#include "chronos/worker/task/task_contract.hpp"

namespace chronos::worker::task {

std::string ToString(TaskResultKind kind) {
  switch (kind) {
    case TaskResultKind::kSuccess:
      return "SUCCESS";
    case TaskResultKind::kRetryableFailure:
      return "RETRYABLE_FAILURE";
    case TaskResultKind::kPermanentFailure:
      return "PERMANENT_FAILURE";
    case TaskResultKind::kTimeout:
      return "TIMEOUT";
    case TaskResultKind::kAbandoned:
      return "ABANDONED";
  }
  return "UNKNOWN";
}

}  // namespace chronos::worker::task
