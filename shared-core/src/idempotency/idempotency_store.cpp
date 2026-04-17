#include "chronos/idempotency/idempotency_store.hpp"

namespace chronos::idempotency {

bool InMemoryIdempotencyStore::TryAcquire(
    const std::string& key,
    const std::string& owner,
    std::chrono::seconds ttl) {
  const auto now = std::chrono::steady_clock::now();

  std::lock_guard<std::mutex> lock(mu_);
  const auto it = locks_.find(key);
  if (it == locks_.end() || it->second.expires_at <= now) {
    locks_[key] = LockEntry{.owner = owner, .expires_at = now + ttl};
    return true;
  }

  return it->second.owner == owner;
}

void InMemoryIdempotencyStore::Release(
    const std::string& key,
    const std::string& owner) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto it = locks_.find(key);
  if (it == locks_.end()) {
    return;
  }

  if (it->second.owner == owner) {
    locks_.erase(it);
  }
}

}  // namespace chronos::idempotency
