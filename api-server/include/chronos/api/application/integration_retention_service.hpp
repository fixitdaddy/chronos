#pragma once

#include <chrono>
#include <memory>

#include "chronos/api/application/integration_idempotency_repository.hpp"

namespace chronos::api::application {

struct RetentionRunResult {
  std::size_t idempotency_removed{0};
  std::size_t dedupe_removed{0};
};

class IntegrationRetentionService {
 public:
  IntegrationRetentionService(
      std::shared_ptr<IIntegrationIdempotencyRepository> idempotency_repository,
      std::shared_ptr<IEventDedupeRepository> event_dedupe_repository);

  RetentionRunResult Run(
      std::chrono::seconds idempotency_ttl,
      std::chrono::seconds dedupe_ttl) const;

 private:
  std::shared_ptr<IIntegrationIdempotencyRepository> idempotency_repository_;
  std::shared_ptr<IEventDedupeRepository> event_dedupe_repository_;
};

}  // namespace chronos::api::application
