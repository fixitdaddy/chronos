#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace chronos::idempotency {

class InMemoryIdempotencyStore {
 public:
  bool TryAcquire(
      const std::string& key,
      const std::string& owner,
      std::chrono::seconds ttl);
  void Release(const std::string& key, const std::string& owner);

 private:
  struct LockEntry {
    std::string owner;
    std::chrono::steady_clock::time_point expires_at;
  };

  std::mutex mu_;
  std::unordered_map<std::string, LockEntry> locks_;
};

}  // namespace chronos::idempotency
