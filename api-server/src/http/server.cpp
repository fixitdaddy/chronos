#include "chronos/api/http/server.hpp"

#include <chrono>
#include <utility>

#include "chronos/api/handlers/dead_letter_handlers.hpp"
#include "chronos/api/handlers/health_handlers.hpp"
#include "chronos/api/handlers/job_handlers.hpp"
#include "chronos/api/handlers/metrics_handlers.hpp"
#include "chronos/api/handlers/schedule_handlers.hpp"
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

  // Redis-backed API rate limit (fail-open if Redis unavailable).
  if (request.path != "/health" && context_->coordination) {
    const auto bucket = std::string("api:global:") + request.method;
    const auto allowed = context_->coordination->TryConsumeRate(bucket, 100, std::chrono::seconds(60));
    if (!allowed) {
      return {.status = 429,
              .body = R"({"error":{"code":"RATE_LIMITED","message":"rate limit exceeded"}})",
              .headers = {{"content-type", "application/json"}, {"x-request-id", request.request_id}}};
    }
  }

  auto response = router_.Handle(request);
  response.headers["x-request-id"] = request.request_id;
  return response;
}

}  // namespace chronos::api::http
