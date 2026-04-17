#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "chronos/worker/task/task_contract.hpp"

namespace chronos::worker::task {

class TaskRegistry {
 public:
  bool Register(const std::string& task_type, TaskHandler handler);
  std::optional<TaskHandler> Resolve(const std::string& task_type) const;

 private:
  mutable std::mutex mu_;
  std::unordered_map<std::string, TaskHandler> handlers_;
};

}  // namespace chronos::worker::task
