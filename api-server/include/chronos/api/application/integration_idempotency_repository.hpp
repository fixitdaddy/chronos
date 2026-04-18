#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace chronos::api::application {

struct IdempotencyRecord {
  std::string tenant_id;
  std::string endpoint;
  std::string idempotency_key;
  std::string canonical_payload_hash;
  std::string response_body;
  std::chrono::system_clock::time_point created_at{};
  std::chrono::system_clock::time_point updated_at{};
};

class IIntegrationIdempotencyRepository {
 public:
  virtual ~IIntegrationIdempotencyRepository() = default;

  virtual std::optional<IdempotencyRecord> Get(
      const std::string& tenant_id,
      const std::string& endpoint,
      const std::string& idempotency_key) = 0;

  virtual bool Put(const IdempotencyRecord& record) = 0;

  virtual std::size_t PurgeExpired(std::chrono::seconds ttl) = 0;
};

class InMemoryIntegrationIdempotencyRepository final : public IIntegrationIdempotencyRepository {
 public:
  std::optional<IdempotencyRecord> Get(
      const std::string& tenant_id,
      const std::string& endpoint,
      const std::string& idempotency_key) override;

  bool Put(const IdempotencyRecord& record) override;

  std::size_t PurgeExpired(std::chrono::seconds ttl) override;

 private:
  static std::string BuildScopeKey(
      const std::string& tenant_id,
      const std::string& endpoint,
      const std::string& idempotency_key);

  std::mutex mu_;
  std::unordered_map<std::string, IdempotencyRecord> by_scope_key_;
};

// Postgres repository scaffold for migration-backed production path.
class PostgresIntegrationIdempotencyRepository final : public IIntegrationIdempotencyRepository {
 public:
  explicit PostgresIntegrationIdempotencyRepository(std::string connection_uri);

  std::optional<IdempotencyRecord> Get(
      const std::string& tenant_id,
      const std::string& endpoint,
      const std::string& idempotency_key) override;

  bool Put(const IdempotencyRecord& record) override;

  std::size_t PurgeExpired(std::chrono::seconds ttl) override;

 private:
  std::string connection_uri_;
};

std::string ComputeCanonicalPayloadHash(const std::string& request_body);

struct EventDedupeRecord {
  std::string tenant_id;
  std::string event_id;
  std::chrono::system_clock::time_point seen_at{};
};

class IEventDedupeRepository {
 public:
  virtual ~IEventDedupeRepository() = default;

  // Returns true if this is the first time event key is seen (not duplicate).
  virtual bool TryMarkSeen(
      const std::string& tenant_id,
      const std::string& event_id) = 0;

  virtual bool IsSeen(
      const std::string& tenant_id,
      const std::string& event_id) = 0;

  virtual std::size_t PurgeExpired(std::chrono::seconds ttl) = 0;
};

class InMemoryEventDedupeRepository final : public IEventDedupeRepository {
 public:
  bool TryMarkSeen(
      const std::string& tenant_id,
      const std::string& event_id) override;

  bool IsSeen(
      const std::string& tenant_id,
      const std::string& event_id) override;

  std::size_t PurgeExpired(std::chrono::seconds ttl) override;

 private:
  static std::string BuildScopeKey(
      const std::string& tenant_id,
      const std::string& event_id);

  std::mutex mu_;
  std::unordered_map<std::string, EventDedupeRecord> seen_by_scope_key_;
};

class PostgresEventDedupeRepository final : public IEventDedupeRepository {
 public:
  explicit PostgresEventDedupeRepository(std::string connection_uri);

  bool TryMarkSeen(
      const std::string& tenant_id,
      const std::string& event_id) override;

  bool IsSeen(
      const std::string& tenant_id,
      const std::string& event_id) override;

  std::size_t PurgeExpired(std::chrono::seconds ttl) override;

 private:
  std::string connection_uri_;
};

}  // namespace chronos::api::application
