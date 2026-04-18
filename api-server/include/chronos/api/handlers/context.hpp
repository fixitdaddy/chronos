#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "chronos/coordination/redis_coordination.hpp"
#include "chronos/persistence/in_memory/in_memory_repositories.hpp"
#include "chronos/persistence/repository.hpp"

namespace chronos::api::application {
class IIntegrationIdempotencyRepository;
class IEventDedupeRepository;
}

namespace chronos::api::handlers {

struct Metrics {
  std::atomic<unsigned long long> requests_total{0};
  std::atomic<unsigned long long> jobs_created_total{0};
  std::atomic<unsigned long long> schedules_created_total{0};
  std::atomic<unsigned long long> execution_queries_total{0};

  // Phase 8 observability
  std::atomic<unsigned long long> dispatch_rate_total{0};
  std::atomic<unsigned long long> success_total{0};
  std::atomic<unsigned long long> failure_total{0};
  std::atomic<unsigned long long> retry_total{0};
  std::atomic<long long> queue_depth{0};
  std::atomic<long long> schedule_lag_ms{0};
  std::atomic<long long> worker_utilization_pct{0};
  std::atomic<long long> task_latency_ms{0};
  std::atomic<long long> db_latency_ms{0};

  // Phase 10 multi-tenant visibility
  std::atomic<unsigned long long> audit_events_total{0};
  mutable std::mutex tenant_mu;
  std::unordered_map<std::string, unsigned long long> tenant_requests_total;
};

struct HandlerContext {
  std::shared_ptr<persistence::IJobRepository> job_repository;
  std::shared_ptr<persistence::IScheduleRepository> schedule_repository;
  std::shared_ptr<persistence::IExecutionRepository> execution_repository;
  std::shared_ptr<persistence::in_memory::InMemoryExecutionRepository> in_memory_execution_repository;
  std::shared_ptr<coordination::IRedisCoordination> coordination;
  std::shared_ptr<Metrics> metrics;
  std::shared_ptr<application::IIntegrationIdempotencyRepository> integration_idempotency_repository;
  std::shared_ptr<application::IEventDedupeRepository> event_dedupe_repository;
  std::string service_name{"api-server"};
};

}  // namespace chronos::api::handlers
