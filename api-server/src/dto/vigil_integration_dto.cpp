#include "chronos/api/dto/vigil_integration_dto.hpp"

#include <cctype>
#include <cstdlib>
#include <string>

namespace chronos::api::dto {
namespace {

std::string ExtractStringValue(const std::string& body, const std::string& key) {
  const auto needle = "\"" + key + "\"";
  const auto key_pos = body.find(needle);
  if (key_pos == std::string::npos) {
    return "";
  }

  const auto colon_pos = body.find(':', key_pos + needle.size());
  if (colon_pos == std::string::npos) {
    return "";
  }

  auto value_start = body.find('"', colon_pos + 1);
  if (value_start == std::string::npos) {
    return "";
  }
  ++value_start;

  const auto value_end = body.find('"', value_start);
  if (value_end == std::string::npos || value_end <= value_start) {
    return "";
  }

  return body.substr(value_start, value_end - value_start);
}

int ExtractIntValue(const std::string& body, const std::string& key, int fallback) {
  const auto needle = "\"" + key + "\"";
  const auto key_pos = body.find(needle);
  if (key_pos == std::string::npos) {
    return fallback;
  }

  const auto colon_pos = body.find(':', key_pos + needle.size());
  if (colon_pos == std::string::npos) {
    return fallback;
  }

  std::size_t pos = colon_pos + 1;
  while (pos < body.size() && std::isspace(static_cast<unsigned char>(body[pos]))) {
    ++pos;
  }

  std::size_t end = pos;
  while (end < body.size() && (std::isdigit(static_cast<unsigned char>(body[end])) || body[end] == '-')) {
    ++end;
  }

  if (end <= pos) {
    return fallback;
  }

  try {
    return std::stoi(body.substr(pos, end - pos));
  } catch (...) {
    return fallback;
  }
}

bool Contains(const std::string& body, const std::string& token) {
  return body.find(token) != std::string::npos;
}

}  // namespace

ParseCreateRemediationJobResult ParseCreateRemediationJobDto(const std::string& body) {
  ParseCreateRemediationJobResult out;
  out.dto.raw_body = body;

  if (body.empty()) {
    out.ok = false;
    out.message = "request body must not be empty";
    return out;
  }

  if (!Contains(body, "\"tenantId\"") ||
      !Contains(body, "\"incident\"") ||
      !Contains(body, "\"action\"") ||
      !Contains(body, "\"target\"") ||
      !Contains(body, "\"execution\"") ||
      !Contains(body, "\"schedule\"") ||
      !Contains(body, "\"audit\"")) {
    out.ok = false;
    out.message = "missing required top-level fields";
    return out;
  }

  out.dto.tenant_id = ExtractStringValue(body, "tenantId");
  out.dto.task_ref = ExtractStringValue(body, "taskRef");
  out.dto.schedule_kind = ExtractStringValue(body, "kind");
  out.dto.run_at = ExtractStringValue(body, "runAt");
  out.dto.cron = ExtractStringValue(body, "cron");
  out.dto.timezone = ExtractStringValue(body, "timezone");
  out.dto.requested_by = ExtractStringValue(body, "requestedBy");
  out.dto.reason = ExtractStringValue(body, "reason");
  out.dto.max_attempts = ExtractIntValue(body, "maxAttempts", 1);
  out.dto.timeout_seconds = ExtractIntValue(body, "timeoutSeconds", 300);

  if (out.dto.tenant_id.empty()) {
    out.ok = false;
    out.message = "tenantId is required";
    return out;
  }

  if (out.dto.task_ref.empty()) {
    out.ok = false;
    out.message = "execution.taskRef is required";
    return out;
  }

  if (out.dto.schedule_kind != "once" && out.dto.schedule_kind != "cron") {
    out.ok = false;
    out.message = "schedule.kind must be one of: once, cron";
    return out;
  }

  if (out.dto.schedule_kind == "once" && out.dto.run_at.empty()) {
    out.ok = false;
    out.message = "schedule.runAt is required for once schedule";
    return out;
  }

  if (out.dto.schedule_kind == "cron" && (out.dto.cron.empty() || out.dto.timezone.empty())) {
    out.ok = false;
    out.message = "schedule.cron and schedule.timezone are required for cron schedule";
    return out;
  }

  if (out.dto.max_attempts < 1) {
    out.ok = false;
    out.message = "execution.retryPolicy.maxAttempts must be >= 1";
    return out;
  }

  if (out.dto.timeout_seconds < 1) {
    out.ok = false;
    out.message = "execution.timeoutSeconds must be >= 1";
    return out;
  }

  out.ok = true;
  out.message = "ok";
  return out;
}

}  // namespace chronos::api::dto
