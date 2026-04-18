#pragma once

#include <string>

namespace chronos::api::dto {

struct CreateRemediationJobDto {
  std::string tenant_id;
  std::string task_ref;
  std::string schedule_kind;
  std::string run_at;
  std::string cron;
  std::string timezone;
  int max_attempts{1};
  int timeout_seconds{300};
  std::string requested_by;
  std::string reason;
  std::string raw_body;
};

struct ParseCreateRemediationJobResult {
  bool ok{false};
  std::string message;
  CreateRemediationJobDto dto;
};

ParseCreateRemediationJobResult ParseCreateRemediationJobDto(const std::string& body);

}  // namespace chronos::api::dto
