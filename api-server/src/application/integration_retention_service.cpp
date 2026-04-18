#include "chronos/api/application/integration_retention_service.hpp"

namespace chronos::api::application {

IntegrationRetentionService::IntegrationRetentionService(
    std::shared_ptr<IIntegrationIdempotencyRepository> idempotency_repository,
    std::shared_ptr<IEventDedupeRepository> event_dedupe_repository)
    : idempotency_repository_(std::move(idempotency_repository)),
      event_dedupe_repository_(std::move(event_dedupe_repository)) {}

RetentionRunResult IntegrationRetentionService::Run(
    std::chrono::seconds idempotency_ttl,
    std::chrono::seconds dedupe_ttl) const {
  RetentionRunResult result;

  if (idempotency_repository_) {
    result.idempotency_removed = idempotency_repository_->PurgeExpired(idempotency_ttl);
  }

  if (event_dedupe_repository_) {
    result.dedupe_removed = event_dedupe_repository_->PurgeExpired(dedupe_ttl);
  }

  return result;
}

}  // namespace chronos::api::application
