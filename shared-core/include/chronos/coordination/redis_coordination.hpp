#pragma once

#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace chronos::coordination {

class IRedisCoordination {
 public:
  virtual ~IRedisCoordination() = default;

  virtual bool TryLock(
      const std::string& key,
      const std::string& owner,
      std::chrono::seconds ttl) = 0;
  virtual bool RefreshLock(
      const std::string& key,
      const std::string& owner,
      std::chrono::seconds ttl) = 0;
  virtual void Unlock(const std::string& key, const std::string& owner) = 0;

  virtual bool TryConsumeRate(
      const std::string& bucket,
      std::size_t max_tokens,
      std::chrono::seconds window) = 0;

  virtual void PutCache(
      const std::string& key,
      const std::string& value,
      std::chrono::seconds ttl) = 0;
  virtual std::optional<std::string> GetCache(const std::string& key) = 0;

  virtual bool TryRegisterDedupe(
      const std::string& key,
      std::chrono::seconds ttl) = 0;

  virtual bool Available() const = 0;
  virtual void SetAvailable(bool available) = 0;
};

class InMemoryRedisCoordination final : public IRedisCoordination {
 public:
  bool TryLock(
      const std::string& key,
      const std::string& owner,
      std::chrono::seconds ttl) override;
  bool RefreshLock(
      const std::string& key,
      const std::string& owner,
      std::chrono::seconds ttl) override;
  void Unlock(const std::string& key, const std::string& owner) override;

  bool TryConsumeRate(
      const std::string& bucket,
      std::size_t max_tokens,
      std::chrono::seconds window) override;

  void PutCache(
      const std::string& key,
      const std::string& value,
      std::chrono::seconds ttl) override;
  std::optional<std::string> GetCache(const std::string& key) override;

  bool TryRegisterDedupe(
      const std::string& key,
      std::chrono::seconds ttl) override;

  bool Available() const override;
  void SetAvailable(bool available) override;

 private:
  struct LockEntry {
    std::string owner;
    std::chrono::steady_clock::time_point expires_at;
  };

  struct CacheEntry {
    std::string value;
    std::chrono::steady_clock::time_point expires_at;
  };

  struct RateBucket {
    std::size_t count{0};
    std::chrono::steady_clock::time_point reset_at;
  };

  mutable std::mutex mu_;
  bool available_{true};
  std::unordered_map<std::string, LockEntry> locks_;
  std::unordered_map<std::string, CacheEntry> cache_;
  std::unordered_map<std::string, RateBucket> buckets_;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> dedupe_;
};

}  // namespace chronos::coordination
