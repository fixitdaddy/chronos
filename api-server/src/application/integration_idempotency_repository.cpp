#include "chronos/api/application/integration_idempotency_repository.hpp"

#include <algorithm>
#include <functional>
#include <sstream>

namespace chronos::api::application {
namespace {

std::string BuildScopeKeyImpl(
    const std::string& tenant_id,
    const std::string& endpoint,
    const std::string& idempotency_key) {
  return tenant_id + ":" + endpoint + ":" + idempotency_key;
}

std::string BuildEventScopeKeyImpl(
    const std::string& tenant_id,
    const std::string& event_id) {
  return tenant_id + ":" + event_id;
}

}  // namespace

std::string InMemoryIntegrationIdempotencyRepository::BuildScopeKey(
    const std::string& tenant_id,
    const std::string& endpoint,
    const std::string& idempotency_key) {
  return BuildScopeKeyImpl(tenant_id, endpoint, idempotency_key);
}

std::optional<IdempotencyRecord> InMemoryIntegrationIdempotencyRepository::Get(
    const std::string& tenant_id,
    const std::string& endpoint,
    const std::string& idempotency_key) {
  const auto key = BuildScopeKey(tenant_id, endpoint, idempotency_key);
  std::lock_guard<std::mutex> lock(mu_);
  const auto it = by_scope_key_.find(key);
  if (it == by_scope_key_.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool InMemoryIntegrationIdempotencyRepository::Put(const IdempotencyRecord& record) {
  const auto key = BuildScopeKey(record.tenant_id, record.endpoint, record.idempotency_key);
  std::lock_guard<std::mutex> lock(mu_);

  auto to_store = record;
  const auto now = std::chrono::system_clock::now();
  if (to_store.created_at.time_since_epoch().count() == 0) {
    to_store.created_at = now;
  }
  to_store.updated_at = now;

  by_scope_key_[key] = to_store;
  return true;
}

std::size_t InMemoryIntegrationIdempotencyRepository::PurgeExpired(std::chrono::seconds ttl) {
  const auto now = std::chrono::system_clock::now();

  std::lock_guard<std::mutex> lock(mu_);
  std::size_t removed = 0;

  for (auto it = by_scope_key_.begin(); it != by_scope_key_.end();) {
    const auto age = now - it->second.updated_at;
    if (age > ttl) {
      it = by_scope_key_.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }

  return removed;
}

PostgresIntegrationIdempotencyRepository::PostgresIntegrationIdempotencyRepository(
    std::string connection_uri)
    : connection_uri_(std::move(connection_uri)) {}

std::optional<IdempotencyRecord> PostgresIntegrationIdempotencyRepository::Get(
    const std::string& tenant_id,
    const std::string& endpoint,
    const std::string& idempotency_key) {
  (void)connection_uri_;
  (void)tenant_id;
  (void)endpoint;
  (void)idempotency_key;
  // TODO: SELECT tenant_id, endpoint, idempotency_key, canonical_payload_hash, response_body, created_at, updated_at
  //       FROM integration_idempotency_keys
  //       WHERE tenant_id=$1 AND endpoint=$2 AND idempotency_key=$3
  return std::nullopt;
}

bool PostgresIntegrationIdempotencyRepository::Put(const IdempotencyRecord& record) {
  (void)connection_uri_;
  (void)record;
  // TODO: INSERT INTO integration_idempotency_keys (...)
  //       ON CONFLICT (tenant_id, endpoint, idempotency_key)
  //       DO UPDATE SET canonical_payload_hash=EXCLUDED.canonical_payload_hash,
  //                     response_body=EXCLUDED.response_body,
  //                     updated_at=NOW()
  return true;
}

std::size_t PostgresIntegrationIdempotencyRepository::PurgeExpired(std::chrono::seconds ttl) {
  (void)connection_uri_;
  (void)ttl;
  // TODO: DELETE FROM integration_idempotency_keys
  //       WHERE updated_at < NOW() - ($1 || ' seconds')::interval
  return 0;
}

std::string ComputeCanonicalPayloadHash(const std::string& request_body) {
  // Canonicalization placeholder: use stable hash of raw body for now.
  // Later, replace with JSON canonical form normalization.
  const auto h = std::hash<std::string>{}(request_body);
  std::ostringstream oss;
  oss << std::hex << h;
  return oss.str();
}

std::string InMemoryEventDedupeRepository::BuildScopeKey(
    const std::string& tenant_id,
    const std::string& event_id) {
  return BuildEventScopeKeyImpl(tenant_id, event_id);
}

bool InMemoryEventDedupeRepository::TryMarkSeen(
    const std::string& tenant_id,
    const std::string& event_id) {
  const auto key = BuildScopeKey(tenant_id, event_id);
  std::lock_guard<std::mutex> lock(mu_);
  const auto it = seen_by_scope_key_.find(key);
  if (it != seen_by_scope_key_.end()) {
    return false;
  }

  seen_by_scope_key_[key] = EventDedupeRecord{
      .tenant_id = tenant_id,
      .event_id = event_id,
      .seen_at = std::chrono::system_clock::now(),
  };
  return true;
}

bool InMemoryEventDedupeRepository::IsSeen(
    const std::string& tenant_id,
    const std::string& event_id) {
  const auto key = BuildScopeKey(tenant_id, event_id);
  std::lock_guard<std::mutex> lock(mu_);
  return seen_by_scope_key_.find(key) != seen_by_scope_key_.end();
}

std::size_t InMemoryEventDedupeRepository::PurgeExpired(std::chrono::seconds ttl) {
  const auto now = std::chrono::system_clock::now();

  std::lock_guard<std::mutex> lock(mu_);
  std::size_t removed = 0;

  for (auto it = seen_by_scope_key_.begin(); it != seen_by_scope_key_.end();) {
    const auto age = now - it->second.seen_at;
    if (age > ttl) {
      it = seen_by_scope_key_.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }

  return removed;
}

PostgresEventDedupeRepository::PostgresEventDedupeRepository(std::string connection_uri)
    : connection_uri_(std::move(connection_uri)) {}

bool PostgresEventDedupeRepository::TryMarkSeen(
    const std::string& tenant_id,
    const std::string& event_id) {
  (void)connection_uri_;
  (void)tenant_id;
  (void)event_id;
  // TODO: INSERT INTO integration_event_dedupe (tenant_id, event_id, seen_at)
  //       VALUES ($1,$2,NOW())
  //       ON CONFLICT (tenant_id, event_id) DO NOTHING
  //       return true when inserted, false when duplicate
  return true;
}

bool PostgresEventDedupeRepository::IsSeen(
    const std::string& tenant_id,
    const std::string& event_id) {
  (void)connection_uri_;
  (void)tenant_id;
  (void)event_id;
  // TODO: SELECT 1 FROM integration_event_dedupe WHERE tenant_id=$1 AND event_id=$2
  return false;
}

std::size_t PostgresEventDedupeRepository::PurgeExpired(std::chrono::seconds ttl) {
  (void)connection_uri_;
  (void)ttl;
  // TODO: DELETE FROM integration_event_dedupe
  //       WHERE seen_at < NOW() - ($1 || ' seconds')::interval
  return 0;
}

}  // namespace chronos::api::application
