#include "chronos/api/handlers/vigil_integration_handlers.hpp"

#include <atomic>
#include <cstring>
#include <string>

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

http::HttpResponse MissingHeader(const std::string& header_name) {
  return {
      .status = 400,
      .body = "{\"error\":{\"code\":\"VALIDATION_ERROR\",\"message\":\"missing required header: " +
              header_name + "\"}}",
      .headers = {{"content-type", "application/json"}},
  };
}

}  // namespace

http::HttpResponse HandleCreateVigilRemediationJob(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  // Contract-mandatory headers for Phase 1.1
  const auto tenant_id = GetHeader(request, "x-tenant-id");
  if (tenant_id.empty()) {
    return MissingHeader("x-tenant-id");
  }

  const auto idempotency_key = GetHeader(request, "idempotency-key");
  if (idempotency_key.empty()) {
    return MissingHeader("idempotency-key");
  }

  const auto vigilance_incident_id = GetHeader(request, "x-vigil-incident-id");
  if (vigilance_incident_id.empty()) {
    return MissingHeader("x-vigil-incident-id");
  }

  const auto vigil_action_id = GetHeader(request, "x-vigil-action-id");
  if (vigil_action_id.empty()) {
    return MissingHeader("x-vigil-action-id");
  }

  // Phase 1.1 contract scaffold response (persistence/mapping hardening in later subphases).
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
      "\"links\":{" 
      "\"job\":\"/v1/jobs/" + chronos_job_id + "\"," 
      "\"execution\":\"/v1/jobs/" + chronos_job_id + "/executions/" + execution_id + "\"," 
      "\"integration\":\"/v1/integrations/vigil/remediation-jobs/" + remediation_job_id + "\""
      "}"
      "}";

  return {
      .status = 202,
      .body = response_body,
      .headers = {
          {"content-type", "application/json"},
          {"idempotency-replayed", "false"},
          {"x-tenant-id", tenant_id},
      },
  };
}

http::HttpResponse HandleGetVigilRemediationJobById(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  const auto tenant_id = GetHeader(request, "x-tenant-id", "default");
  const auto it = request.path_params.find("id");
  if (it == request.path_params.end() || it->second.empty()) {
    return {
        .status = 400,
        .body = R"({"error":{"code":"VALIDATION_ERROR","message":"id path param missing"}})",
        .headers = {{"content-type", "application/json"}},
    };
  }

  const auto body =
      "{"
      "\"remediationJobId\":\"" + it->second + "\"," 
      "\"tenantId\":\"" + tenant_id + "\"," 
      "\"status\":\"accepted\"," 
      "\"state\":\"scheduled\"," 
      "\"chronosJobId\":\"job-lookup-pending\"," 
      "\"executionId\":\"exec-lookup-pending\""
      "}";

  return {
      .status = 200,
      .body = body,
      .headers = {{"content-type", "application/json"}},
  };
}

http::HttpResponse HandleCancelVigilRemediationJob(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  const auto tenant_id = GetHeader(request, "x-tenant-id", "default");

  std::string remediation_job_id;
  if (const auto id_it = request.path_params.find("id");
      id_it != request.path_params.end() && !id_it->second.empty()) {
    remediation_job_id = id_it->second;
  } else if (const auto id_cancel_it = request.path_params.find("id:cancel");
             id_cancel_it != request.path_params.end() && !id_cancel_it->second.empty()) {
    remediation_job_id = id_cancel_it->second;
  }

  if (remediation_job_id.empty()) {
    return {
        .status = 400,
        .body = R"({"error":{"code":"VALIDATION_ERROR","message":"id path param missing"}})",
        .headers = {{"content-type", "application/json"}},
    };
  }

  if (request.path.rfind(":cancel") == std::string::npos) {
    return {
        .status = 400,
        .body = R"({"error":{"code":"VALIDATION_ERROR","message":"expected path suffix ':cancel'"}})",
        .headers = {{"content-type", "application/json"}},
    };
  }

  constexpr const char* kCancelSuffix = ":cancel";
  if (remediation_job_id.size() > std::char_traits<char>::length(kCancelSuffix) &&
      remediation_job_id.rfind(kCancelSuffix) == remediation_job_id.size() - std::char_traits<char>::length(kCancelSuffix)) {
    remediation_job_id = remediation_job_id.substr(0, remediation_job_id.size() - std::char_traits<char>::length(kCancelSuffix));
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
      .headers = {{"content-type", "application/json"}},
  };
}

http::HttpResponse HandleGetVigilActionStatus(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  const auto tenant_id = GetHeader(request, "x-tenant-id", "default");
  const auto it = request.path_params.find("vigilActionId");
  if (it == request.path_params.end() || it->second.empty()) {
    return {
        .status = 400,
        .body = R"({"error":{"code":"VALIDATION_ERROR","message":"vigilActionId path param missing"}})",
        .headers = {{"content-type", "application/json"}},
    };
  }

  const auto body =
      "{"
      "\"vigilActionId\":\"" + it->second + "\"," 
      "\"tenantId\":\"" + tenant_id + "\"," 
      "\"status\":\"scheduled\"," 
      "\"chronosJobId\":\"job-lookup-pending\"," 
      "\"executionId\":\"exec-lookup-pending\""
      "}";

  return {
      .status = 200,
      .body = body,
      .headers = {{"content-type", "application/json"}},
  };
}

}  // namespace chronos::api::handlers
