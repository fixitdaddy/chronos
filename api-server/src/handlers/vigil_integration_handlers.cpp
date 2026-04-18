#include "chronos/api/handlers/vigil_integration_handlers.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "chronos/time/clock.hpp"

namespace chronos::api::handlers {
namespace {

std::string NextId(const std::string& prefix) {
  static std::atomic<unsigned long long> counter{1};
  return prefix + "-" + std::to_string(counter.fetch_add(1));
}

std::string GetHeader(
    const http::HttpRequest& request,
    const std::string& key,
    const std::string& fallback = "") {
  const auto it = request.headers.find(key);
  if (it == request.headers.end() || it->second.empty()) {
    return fallback;
  }
  return it->second;
}

std::string EscapeJson(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (const auto c : value) {
    if (c == '"') {
      out += "\\\"";
    } else if (c == '\\') {
      out += "\\\\";
    } else {
      out.push_back(c);
    }
  }
  return out;
}

http::HttpResponse BuildError(
    int status,
    const std::string& code,
    const std::string& message,
    const std::string& request_id,
    const std::string& correlation_id,
    bool retryable) {
  const auto now = time::ToIso8601(time::UtcNow());
  return {
      .status = status,
      .body =
          "{"
          "\"error\":{"
          "\"code\":\"" + EscapeJson(code) + "\","
          "\"message\":\"" + EscapeJson(message) + "\","
          "\"httpStatus\":" + std::to_string(status) + ","
          "\"retryable\":" + std::string(retryable ? "true" : "false") +
          "},"
          "\"requestId\":\"" + EscapeJson(request_id) + "\"," 
          "\"correlationId\":\"" + EscapeJson(correlation_id) + "\"," 
          "\"timestamp\":\"" + now + "\""
          "}",
      .headers = {
          {"content-type", "application/json"},
          {"x-request-id", request_id},
          {"x-correlation-id", correlation_id},
      },
  };
}

std::optional<http::HttpResponse> ValidateMandatoryHeaders(const http::HttpRequest& request) {
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  static const std::vector<std::string> kRequiredHeaders{
      "x-tenant-id",
      "idempotency-key",
      "x-request-id",
      "x-correlation-id",
      "x-vigil-incident-id",
      "x-vigil-action-id",
  };

  for (const auto& h : kRequiredHeaders) {
    if (GetHeader(request, h).empty()) {
      return BuildError(
          400,
          "CHRONOS_VALIDATION_ERROR",
          "missing required header: " + h,
          request_id,
          correlation_id,
          false);
    }
  }
  return std::nullopt;
}

std::optional<http::HttpResponse> ValidateCreatePayloadShape(const http::HttpRequest& request) {
  // Lightweight JSON schema enforcement (Phase 1.2) without external parser dependency.
  // Deep semantic validation moves to typed DTO layer in later phases.
  const auto& body = request.body;
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  if (body.empty()) {
    return BuildError(
        400,
        "CHRONOS_VALIDATION_ERROR",
        "request body must be valid JSON object",
        request_id,
        correlation_id,
        false);
  }

  static const std::vector<std::string> kRequiredKeys{
      "\"tenantId\"",
      "\"incident\"",
      "\"action\"",
      "\"target\"",
      "\"execution\"",
      "\"schedule\"",
      "\"audit\"",
  };

  for (const auto& key : kRequiredKeys) {
    if (body.find(key) == std::string::npos) {
      return BuildError(
          400,
          "CHRONOS_VALIDATION_ERROR",
          "request schema violation: missing required field " + key,
          request_id,
          correlation_id,
          false);
    }
  }

  return std::nullopt;
}

std::string NormalizedActionStateFromExecutionState(const std::string& execution_state) {
  if (execution_state == "PENDING") return "queued";
  if (execution_state == "DISPATCHED") return "dispatched";
  if (execution_state == "RUNNING") return "in_progress";
  if (execution_state == "RETRY_PENDING") return "retrying";
  if (execution_state == "SUCCEEDED") return "succeeded";
  if (execution_state == "FAILED") return "failed";
  if (execution_state == "DEAD_LETTER") return "dead_lettered";
  return "unknown";
}

std::string IncidentImpactFromExecutionState(const std::string& execution_state) {
  if (execution_state == "SUCCEEDED") {
    return "resolved_candidate";
  }
  if (execution_state == "FAILED" || execution_state == "DEAD_LETTER") {
    return "escalated";
  }
  return "open";
}

}  // namespace

http::HttpResponse HandleCreateVigilRemediationJob(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  struct IdempotencyEntry {
    std::string payload_fingerprint;
    std::string response_body;
  };

  static std::mutex idem_mu;
  static std::unordered_map<std::string, IdempotencyEntry> idem_store;

  context->metrics->requests_total.fetch_add(1);

  if (const auto error = ValidateMandatoryHeaders(request); error.has_value()) {
    return *error;
  }
  if (const auto error = ValidateCreatePayloadShape(request); error.has_value()) {
    return *error;
  }

  const auto tenant_id = GetHeader(request, "x-tenant-id");
  const auto idempotency_key = GetHeader(request, "idempotency-key");
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  const auto scope_key = tenant_id + ":POST:/v1/integrations/vigil/remediation-jobs:" + idempotency_key;
  const auto payload_fingerprint = std::to_string(std::hash<std::string>{}(request.body));

  {
    std::lock_guard<std::mutex> lock(idem_mu);
    const auto it = idem_store.find(scope_key);
    if (it != idem_store.end()) {
      if (it->second.payload_fingerprint != payload_fingerprint) {
        return BuildError(
            409,
            "CHRONOS_IDEMPOTENCY_MISMATCH",
            "idempotency key reused with different payload",
            request_id,
            correlation_id,
            false);
      }

      return {
          .status = 202,
          .body = it->second.response_body,
          .headers = {
              {"content-type", "application/json"},
              {"idempotency-replayed", "true"},
              {"x-tenant-id", tenant_id},
              {"x-request-id", request_id},
              {"x-correlation-id", correlation_id},
          },
      };
    }
  }

  const auto remediation_job_id = NextId("rj");
  const auto chronos_job_id = NextId("job");
  const auto execution_id = NextId("exec");
  const auto now = time::ToIso8601(time::UtcNow());

  const auto response_body =
      "{"
      "\"remediationJobId\":\"" + remediation_job_id + "\","
      "\"chronosJobId\":\"" + chronos_job_id + "\","
      "\"executionId\":\"" + execution_id + "\","
      "\"tenantId\":\"" + tenant_id + "\","
      "\"status\":\"accepted\","
      "\"state\":\"scheduled\","
      "\"createdAt\":\"" + now + "\","
      "\"requestId\":\"" + request_id + "\","
      "\"correlationId\":\"" + correlation_id + "\","
      "\"links\":{"
      "\"job\":\"/v1/jobs/" + chronos_job_id + "\","
      "\"execution\":\"/v1/jobs/" + chronos_job_id + "/executions/" + execution_id + "\","
      "\"integration\":\"/v1/integrations/vigil/remediation-jobs/" + remediation_job_id + "\""
      "}"
      "}";

  {
    std::lock_guard<std::mutex> lock(idem_mu);
    idem_store[scope_key] = IdempotencyEntry{
        .payload_fingerprint = payload_fingerprint,
        .response_body = response_body,
    };
  }

  return {
      .status = 202,
      .body = response_body,
      .headers = {
          {"content-type", "application/json"},
          {"idempotency-replayed", "false"},
          {"x-tenant-id", tenant_id},
          {"x-request-id", request_id},
          {"x-correlation-id", correlation_id},
      },
  };
}

http::HttpResponse HandleGetVigilRemediationJobById(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  if (const auto error = ValidateMandatoryHeaders(request); error.has_value()) {
    return *error;
  }

  const auto tenant_id = GetHeader(request, "x-tenant-id");
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  const auto it = request.path_params.find("id");
  if (it == request.path_params.end() || it->second.empty()) {
    return BuildError(
        400,
        "CHRONOS_VALIDATION_ERROR",
        "id path param missing",
        request_id,
        correlation_id,
        false);
  }

  const auto execution_state = std::string("DISPATCHED");
  const auto body =
      "{"
      "\"remediationJobId\":\"" + it->second + "\","
      "\"tenantId\":\"" + tenant_id + "\","
      "\"status\":\"accepted\","
      "\"state\":\"scheduled\","
      "\"executionState\":\"" + execution_state + "\"," 
      "\"vigilActionState\":\"" + NormalizedActionStateFromExecutionState(execution_state) + "\"," 
      "\"incidentImpact\":\"" + IncidentImpactFromExecutionState(execution_state) + "\"," 
      "\"chronosJobId\":\"job-lookup-pending\"," 
      "\"executionId\":\"exec-lookup-pending\""
      "}";

  return {
      .status = 200,
      .body = body,
      .headers = {
          {"content-type", "application/json"},
          {"x-request-id", request_id},
          {"x-correlation-id", correlation_id},
      },
  };
}

http::HttpResponse HandleCancelVigilRemediationJob(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  if (const auto error = ValidateMandatoryHeaders(request); error.has_value()) {
    return *error;
  }

  const auto tenant_id = GetHeader(request, "x-tenant-id");
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  std::string remediation_job_id;
  if (const auto id_it = request.path_params.find("id");
      id_it != request.path_params.end() && !id_it->second.empty()) {
    remediation_job_id = id_it->second;
  } else if (const auto id_cancel_it = request.path_params.find("id:cancel");
             id_cancel_it != request.path_params.end() && !id_cancel_it->second.empty()) {
    remediation_job_id = id_cancel_it->second;
  }

  if (remediation_job_id.empty()) {
    return BuildError(
        400,
        "CHRONOS_VALIDATION_ERROR",
        "id path param missing",
        request_id,
        correlation_id,
        false);
  }

  if (request.path.rfind(":cancel") == std::string::npos) {
    return BuildError(
        400,
        "CHRONOS_VALIDATION_ERROR",
        "expected path suffix ':cancel'",
        request_id,
        correlation_id,
        false);
  }

  constexpr const char* kCancelSuffix = ":cancel";
  const auto suffix_len = std::char_traits<char>::length(kCancelSuffix);
  if (remediation_job_id.size() > suffix_len &&
      remediation_job_id.rfind(kCancelSuffix) == remediation_job_id.size() - suffix_len) {
    remediation_job_id = remediation_job_id.substr(0, remediation_job_id.size() - suffix_len);
  }

  const auto body =
      "{"
      "\"remediationJobId\":\"" + remediation_job_id + "\","
      "\"tenantId\":\"" + tenant_id + "\","
      "\"status\":\"cancel_requested\""
      "}";

  return {
      .status = 202,
      .body = body,
      .headers = {
          {"content-type", "application/json"},
          {"x-request-id", request_id},
          {"x-correlation-id", correlation_id},
      },
  };
}

http::HttpResponse HandleGetVigilActionStatus(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  if (const auto error = ValidateMandatoryHeaders(request); error.has_value()) {
    return *error;
  }

  const auto tenant_id = GetHeader(request, "x-tenant-id");
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  const auto it = request.path_params.find("vigilActionId");
  if (it == request.path_params.end() || it->second.empty()) {
    return BuildError(
        400,
        "CHRONOS_VALIDATION_ERROR",
        "vigilActionId path param missing",
        request_id,
        correlation_id,
        false);
  }

  const auto execution_state = std::string("RUNNING");
  const auto body =
      "{"
      "\"vigilActionId\":\"" + it->second + "\","
      "\"tenantId\":\"" + tenant_id + "\","
      "\"executionState\":\"" + execution_state + "\"," 
      "\"status\":\"" + NormalizedActionStateFromExecutionState(execution_state) + "\"," 
      "\"incidentImpact\":\"" + IncidentImpactFromExecutionState(execution_state) + "\"," 
      "\"chronosJobId\":\"job-lookup-pending\"," 
      "\"executionId\":\"exec-lookup-pending\""
      "}";

  return {
      .status = 200,
      .body = body,
      .headers = {
          {"content-type", "application/json"},
          {"x-request-id", request_id},
          {"x-correlation-id", correlation_id},
      },
  };
}

}  // namespace chronos::api::handlers
