#include "chronos/api/http/server.hpp"

#include <chrono>
#include <mutex>
#include <string>
#include <utility>

#include "chronos/api/handlers/dead_letter_handlers.hpp"
#include "chronos/api/handlers/health_handlers.hpp"
#include "chronos/api/handlers/job_handlers.hpp"
#include "chronos/api/handlers/metrics_handlers.hpp"
#include "chronos/api/handlers/schedule_handlers.hpp"
#include "chronos/api/handlers/vigil_integration_handlers.hpp"
#include "chronos/api/middleware/request_id_middleware.hpp"
#include "chronos/api/observability/logger.hpp"

namespace chronos::api::http {

ApiServer::ApiServer(
    std::shared_ptr<handlers::HandlerContext> context,
    std::string bearer_token)
    : context_(std::move(context)),
      auth_middleware_(std::move(bearer_token)) {
  router_.Register("POST", "/jobs", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleCreateJob(req, ctx);
  });

  router_.Register("POST", "/schedules", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleCreateSchedule(req, ctx);
  });

  router_.Register("GET", "/jobs/:id", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleGetJobById(req, ctx);
  });

  router_.Register("GET", "/jobs/:id/executions", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleGetJobExecutions(req, ctx);
  });

  router_.Register("GET", "/health", [](const HttpRequest& req) {
    return handlers::HandleHealth(req);
  });

  router_.Register("GET", "/metrics", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleMetrics(req, ctx);
  });

  // Phase 1.1 vigil -> chronos integration contract routes.
  router_.Register("POST", "/v1/integrations/vigil/remediation-jobs", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleCreateVigilRemediationJob(req, ctx);
  });

  router_.Register("GET", "/v1/integrations/vigil/remediation-jobs/:id", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleGetVigilRemediationJobById(req, ctx);
  });

  router_.Register("POST", "/v1/integrations/vigil/remediation-jobs/:id:cancel", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleCancelVigilRemediationJob(req, ctx);
  });

  router_.Register("GET", "/v1/integrations/vigil/actions/:vigilActionId", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleGetVigilActionStatus(req, ctx);
  });

  router_.Register("GET", "/dead-letter/executions", [ctx = context_](const HttpRequest& req) {
    if (!ctx->in_memory_execution_repository) {
      return HttpResponse{.status = 501,
                          .body = R"({"error":{"code":"NOT_IMPLEMENTED","message":"dead-letter inspection only available in local in-memory mode"}})",
                          .headers = {{"content-type", "application/json"}}};
    }
    return handlers::HandleGetDeadLetterExecutions(req, ctx, ctx->in_memory_execution_repository);
  });

  router_.Register("POST", "/dead-letter/executions/:execution_id/quarantine", [ctx = context_](const HttpRequest& req) {
    if (!ctx->in_memory_execution_repository) {
      return HttpResponse{.status = 501,
                          .body = R"({"error":{"code":"NOT_IMPLEMENTED","message":"quarantine endpoint only available in local in-memory mode"}})",
                          .headers = {{"content-type", "application/json"}}};
    }
    return handlers::HandleQuarantineExecution(req, ctx->in_memory_execution_repository);
  });
}

HttpResponse ApiServer::Handle(HttpRequest request) const {
  request.request_id = middleware::RequestIdMiddleware::ResolveOrGenerate(request);

  observability::Log(
      observability::LogLevel::kInfo,
      context_->service_name,
      request.request_id,
      "incoming_request",
      "{\"method\":\"" + request.method + "\",\"path\":\"" + request.path + "\"}");

  if (request.path != "/health" && !auth_middleware_.IsAuthorized(request)) {
    return {.status = 401,
            .body = R"({"error":{"code":"UNAUTHORIZED","message":"missing or invalid bearer token"}})",
            .headers = {{"content-type", "application/json"}, {"x-request-id", request.request_id}}};
  }

  // Phase 10: tenant extraction + namespace isolation guard.
  const auto tenant_it = request.headers.find("x-tenant-id");
  const auto tenant_id = (tenant_it != request.headers.end() && !tenant_it->second.empty())
                             ? tenant_it->second
                             : std::string("default");

  // Simple namespace policy: tenant header required for non-health endpoints in strict mode.
  const auto strict_it = request.headers.find("x-chronos-strict-tenant");
  const auto strict_tenant = (strict_it != request.headers.end() && strict_it->second == "true");
  if (request.path != "/health" && strict_tenant && tenant_id == "default") {
    return {.status = 403,
            .body = R"({"error":{"code":"FORBIDDEN","message":"tenant namespace required"}})",
            .headers = {{"content-type", "application/json"}, {"x-request-id", request.request_id}}};
  }

  if (context_->metrics) {
    std::lock_guard<std::mutex> lock(context_->metrics->tenant_mu);
    context_->metrics->tenant_requests_total[tenant_id] += 1;
  }

  // Redis-backed multi-tenant quota (fail-open if Redis unavailable).
  if (request.path != "/health" && context_->coordination) {
    const auto bucket = std::string("api:tenant:") + tenant_id + ":" + request.method;
    const auto allowed = context_->coordination->TryConsumeRate(bucket, 100, std::chrono::seconds(60));
    if (!allowed) {
      return {.status = 429,
              .body = R"({"error":{"code":"RATE_LIMITED","message":"tenant quota exceeded"}})",
              .headers = {{"content-type", "application/json"}, {"x-request-id", request.request_id}}};
    }
  }

  auto response = router_.Handle(request);
  response.headers["x-request-id"] = request.request_id;

  // Phase 10: lightweight audit event emission (structured, searchable).
  observability::Log(
      observability::LogLevel::kInfo,
      context_->service_name,
      request.request_id,
      "audit_http_request",
      "{\"tenant_id\":\"" + tenant_id +
          "\",\"method\":\"" + request.method +
          "\",\"path\":\"" + request.path +
          "\",\"status\":" + std::to_string(response.status) + "}");

  if (context_->metrics) {
    context_->metrics->audit_events_total.fetch_add(1);
  }

  return response;
}

}  // namespace chronos::api::http
