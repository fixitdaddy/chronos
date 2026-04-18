#pragma once

#include <memory>
#include <optional>
#include <string>

#include "chronos/api/dto/vigil_integration_dto.hpp"
#include "chronos/api/handlers/context.hpp"

namespace chronos::api::application {

struct CreateRemediationJobResult {
  bool ok{false};
  std::string message;
  std::string remediation_job_id;
  std::string chronos_job_id;
  std::string schedule_id;
  std::string execution_id;
  std::string state;
};

struct IntegrationLookupRecord {
  std::string remediation_job_id;
  std::string tenant_id;
  std::string vigil_incident_id;
  std::string vigil_action_id;
  std::string request_id;
  std::string correlation_id;
  std::string chronos_job_id;
  std::string schedule_id;
  std::string execution_id;
  std::string execution_state;
};

struct LookupRemediationJobResult {
  bool ok{false};
  std::string message;
  IntegrationLookupRecord record;
};

struct LookupVigilActionResult {
  bool ok{false};
  std::string message;
  IntegrationLookupRecord record;
};

struct CancelRemediationJobResult {
  bool ok{false};
  bool cancelled{false};
  std::string message;
  std::string remediation_job_id;
  std::string tenant_id;
  std::string resulting_execution_state;
};

class VigilRemediationService {
 public:
  explicit VigilRemediationService(std::shared_ptr<handlers::HandlerContext> context);

  CreateRemediationJobResult CreateRemediationJob(
      const dto::CreateRemediationJobDto& dto,
      const std::string& tenant_id,
      const std::string& vigil_incident_id,
      const std::string& vigil_action_id,
      const std::string& request_id,
      const std::string& correlation_id,
      const std::string& idempotency_key) const;

  LookupRemediationJobResult GetRemediationJobById(
      const std::string& remediation_job_id,
      const std::string& tenant_id) const;

  LookupVigilActionResult GetByVigilActionId(
      const std::string& vigil_action_id,
      const std::string& tenant_id) const;

  CancelRemediationJobResult CancelRemediationJob(
      const std::string& remediation_job_id,
      const std::string& tenant_id,
      const std::string& request_id,
      const std::string& correlation_id) const;

 private:
  std::shared_ptr<handlers::HandlerContext> context_;
};

}  // namespace chronos::api::application
