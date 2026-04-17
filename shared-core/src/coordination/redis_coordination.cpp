#include "chronos/coordination/redis_coordination.hpp"

namespace chronos::coordination {

bool InMemoryRedisCoordination::TryLock(
    const std::string& key,
    const std::string& owner,
    std::chrono::seconds ttl) {
  std::lock_guard<std::mutex> lock(mu_);
  if (!available_) {
    return false;
  }

  const auto now = std::chrono::steady_clock::now();
  auto it = locks_.find(key);
  if (it == locks_.end() || it->second.expires_at <= now) {
    locks_[key] = LockEntry{.owner = owner, .expires_at = now + ttl};
    return true;
  }

  return it->second.owner == owner;
}

bool InMemoryRedisCoordination::RefreshLock(
    const std::string& key,
    const std::string& owner,
    std::chrono::seconds ttl) {
  std::lock_guard<std::mutex> lock(mu_);
  if (!available_) {
    return false;
  }

  auto it = locks_.find(key);
  if (it == locks_.end()) {
    return false;
  }
  if (it->second.owner != owner) {
    return false;
  }

  it->second.expires_at = std::chrono::steady_clock::now() + ttl;
  return true;
}

void InMemoryRedisCoordination::Unlock(
    const std::string& key,
    const std::string& owner) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = locks_.find(key);
  if (it != locks_.end() && it->second.owner == owner) {
    locks_.erase(it);
  }
}

bool InMemoryRedisCoordination::TryConsumeRate(
    const std::string& bucket,
    std::size_t max_tokens,
    std::chrono::seconds window) {
  std::lock_guard<std::mutex> lock(mu_);
  if (!available_) {
    return true;  // fail-open: Redis is optimization aid.
  }

  const auto now = std::chrono::steady_clock::now();
  auto& b = buckets_[bucket];
  if (b.reset_at <= now) {
    b.count = 0;
    b.reset_at = now + window;
  }

  if (b.count >= max_tokens) {
    return false;
  }

  ++b.count;
  return true;
}

void InMemoryRedisCoordination::PutCache(
    const std::string& key,
    const std::string& value,
    std::chrono::seconds ttl) {
  std::lock_guard<std::mutex> lock(mu_);
  if (!available_) {
    return;
  }
  cache_[key] = CacheEntry{.value = value, .expires_at = std::chrono::steady_clock::now() + ttl};
}

std::optional<std::string> InMemoryRedisCoordination::GetCache(const std::string& key) {
  std::lock_guard<std::mutex> lock(mu_);
  if (!available_) {
    return std::nullopt;
  }

  auto it = cache_.find(key);
  if (it == cache_.end()) {
    return std::nullopt;
  }

  if (it->second.expires_at <= std::chrono::steady_clock::now()) {
    cache_.erase(it);
    return std::nullopt;
  }

  return it->second.value;
}

bool InMemoryRedisCoordination::TryRegisterDedupe(
    const std::string& key,
    std::chrono::seconds ttl) {
  std::lock_guard<std::mutex> lock(mu_);
  if (!available_) {
    return true;  // fail-open: DB is still source of truth.
  }

  const auto now = std::chrono::steady_clock::now();
  auto it = dedupe_.find(key);
  if (it != dedupe_.end() && it->second > now) {
    return false;
  }

  dedupe_[key] = now + ttl;
  return true;
}

bool InMemoryRedisCoordination::Available() const {
  std::lock_guard<std::mutex> lock(mu_);
  return available_;
}

void InMemoryRedisCoordination::SetAvailable(bool available) {
  std::lock_guard<std::mutex> lock(mu_);
  available_ = available;
}

}  // namespace chronos::coordination
