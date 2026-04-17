#include "chronos/worker/task/task_registry.hpp"

#include <utility>

namespace chronos::worker::task {

bool TaskRegistry::Register(const std::string& task_type, TaskHandler handler) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto [_, inserted] = handlers_.emplace(task_type, std::move(handler));
  return inserted;
}

std::optional<TaskHandler> TaskRegistry::Resolve(const std::string& task_type) const {
  std::lock_guard<std::mutex> lock(mu_);
  const auto it = handlers_.find(task_type);
  if (it == handlers_.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace chronos::worker::task
