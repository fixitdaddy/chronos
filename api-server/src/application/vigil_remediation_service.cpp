#include "chronos/api/application/vigil_remediation_service.hpp"

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "chronos/domain/models.hpp"
#include "chronos/serialization/enum_codec.hpp"
#include "chronos/time/clock.hpp"

namespace chronos::api::application {
namespace {

struct IntegrationMetadata {
  std::string remediation_job_id;
  std::string tenant_id;
  std::string vigil_incident_id;
  std::string vigil_action_id;
  std::string request_id;
  std::string correlation_id;
  std::string idempotency_key;
  std::string chronos_job_id;
  std::string schedule_id;
  std::string execution_id;
};

std::string NextId(const std::string& prefix) {
  static std::atomic<unsigned long long> counter{10000};
  return prefix + "-" + std::to_string(counter.fetch_add(1));
}

std::mutex g_metadata_mu;
std::unordered_map<std::string, IntegrationMetadata> g_by_remediation_job_id;
std::unordered_map<std::string, std::string> g_by_vigil_action_id;   // vigil_action_id -> remediation_job_id
std::unordered_map<std::string, std::string> g_by_execution_id;      // execution_id -> remediation_job_id
std::unordered_map<std::string, bool> g_cancel_requested_by_remediation_job_id;

std::optional<IntegrationMetadata> FindByRemediationJobIdLocked(const std::string& remediation_job_id) {
  const auto it = g_by_remediation_job_id.find(remediation_job_id);
  if (it == g_by_remediation_job_id.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool IsCancellableExecutionState(domain::ExecutionState state) {
  return state == domain::ExecutionState::kPending ||
         state == domain::ExecutionState::kDispatched ||
         state == domain::ExecutionState::kRetryPending;
}

}  // namespace

VigilRemediationService::VigilRemediationService(std::shared_ptr<handlers::HandlerContext> context)
    : context_(std::move(context)) {}

CreateRemediationJobResult VigilRemediationService::CreateRemediationJob(
    const dto::CreateRemediationJobDto& dto,
    const std::string& tenant_id,
    const std::string& vigil_incident_id,
    const std::string& vigil_action_id,
    const std::string& request_id,
    const std::string& correlation_id,
    const std::string& idempotency_key) const {
  CreateRemediationJobResult result;

  if (!context_ || !context_->job_repository || !context_->schedule_repository || !context_->execution_repository) {
    result.ok = false;
    result.message = "service dependencies unavailable";
    return result;
  }

  const auto now = time::UtcNow();

  domain::Job job;
  job.job_id = NextId("job");
  job.name = "vigil-remediation";
  job.task_type = dto.task_ref;
  job.payload_json = dto.raw_body;
  job.priority = 0;
  job.queue_name = "main_queue";
  job.max_execution_time_seconds = dto.timeout_seconds;
  job.idempotency_key = idempotency_key;
  job.state = domain::JobState::kActive;
  job.created_at = now;
  job.updated_at = now;

  if (!context_->job_repository->CreateJob(job)) {
    result.ok = false;
    result.message = "failed to create job";
    return result;
  }

  domain::JobSchedule schedule;
  schedule.schedule_id = NextId("schedule");
  schedule.job_id = job.job_id;
  schedule.schedule_type = dto.schedule_kind == "cron"
                               ? domain::ScheduleType::kCron
                               : domain::ScheduleType::kOneTime;

  if (dto.schedule_kind == "cron") {
    schedule.cron_expression = dto.cron;
    schedule.timezone = dto.timezone.empty() ? "UTC" : dto.timezone;
    schedule.next_run_at = now;
  } else {
    schedule.run_at = now;
    schedule.timezone = "UTC";
    schedule.next_run_at = now;
  }

  schedule.misfire_policy = "FIRE_ONCE";
  schedule.active = true;
  schedule.created_at = now;
  schedule.updated_at = now;

  if (!context_->schedule_repository->UpsertSchedule(schedule)) {
    result.ok = false;
    result.message = "failed to create schedule";
    return result;
  }

  domain::JobExecution execution;
  execution.execution_id = NextId("exec");
  execution.job_id = job.job_id;
  execution.schedule_id = schedule.schedule_id;
  execution.execution_number = 1;
  execution.scheduled_at = now;
  execution.state = domain::ExecutionState::kPending;
  execution.max_attempts = dto.max_attempts;
  execution.idempotency_key = idempotency_key;
  execution.result_summary_json =
      std::string("{\"tenantId\":\"") + tenant_id +
      "\",\"vigilIncidentId\":\"" + vigil_incident_id +
      "\",\"vigilActionId\":\"" + vigil_action_id +
      "\",\"requestId\":\"" + request_id +
      "\",\"correlationId\":\"" + correlation_id + "\"}";
  execution.created_at = now;
  execution.updated_at = now;

  if (!context_->execution_repository->CreateExecution(execution)) {
    result.ok = false;
    result.message = "failed to create execution";
    return result;
  }

  if (!context_->execution_repository->TransitionExecutionState(
          execution.execution_id,
          domain::ExecutionState::kPending,
          domain::ExecutionState::kDispatched,
          std::nullopt,
          std::nullopt,
          std::optional<std::string>("api-integration"))) {
    result.ok = false;
    result.message = "failed to move execution to dispatched";
    return result;
  }

  const auto remediation_job_id = NextId("rj");

  {
    std::lock_guard<std::mutex> lock(g_metadata_mu);
    IntegrationMetadata meta;
    meta.remediation_job_id = remediation_job_id;
    meta.tenant_id = tenant_id;
    meta.vigil_incident_id = vigil_incident_id;
    meta.vigil_action_id = vigil_action_id;
    meta.request_id = request_id;
    meta.correlation_id = correlation_id;
    meta.idempotency_key = idempotency_key;
    meta.chronos_job_id = job.job_id;
    meta.schedule_id = schedule.schedule_id;
    meta.execution_id = execution.execution_id;

    g_by_remediation_job_id[remediation_job_id] = meta;
    g_by_vigil_action_id[vigil_action_id] = remediation_job_id;
    g_by_execution_id[execution.execution_id] = remediation_job_id;
  }

  result.ok = true;
  result.message = "ok";
  result.remediation_job_id = remediation_job_id;
  result.chronos_job_id = job.job_id;
  result.schedule_id = schedule.schedule_id;
  result.execution_id = execution.execution_id;
  result.state = "scheduled";

  {
    std::lock_guard<std::mutex> lock(g_metadata_mu);
    g_cancel_requested_by_remediation_job_id[remediation_job_id] = false;
  }

  return result;
}

LookupRemediationJobResult VigilRemediationService::GetRemediationJobById(
    const std::string& remediation_job_id,
    const std::string& tenant_id) const {
  LookupRemediationJobResult result;

  if (!context_ || !context_->execution_repository) {
    result.ok = false;
    result.message = "service dependencies unavailable";
    return result;
  }

  std::optional<IntegrationMetadata> meta;
  {
    std::lock_guard<std::mutex> lock(g_metadata_mu);
    meta = FindByRemediationJobIdLocked(remediation_job_id);
  }

  if (!meta.has_value()) {
    result.ok = false;
    result.message = "remediation job not found";
    return result;
  }

  if (meta->tenant_id != tenant_id) {
    result.ok = false;
    result.message = "tenant mismatch";
    return result;
  }

  const auto execution = context_->execution_repository->GetExecutionById(meta->execution_id);
  if (!execution.has_value()) {
    result.ok = false;
    result.message = "execution not found";
    return result;
  }

  bool cancel_requested = false;
  {
    std::lock_guard<std::mutex> lock(g_metadata_mu);
    const auto it_cancel = g_cancel_requested_by_remediation_job_id.find(meta->remediation_job_id);
    if (it_cancel != g_cancel_requested_by_remediation_job_id.end()) {
      cancel_requested = it_cancel->second;
    }
  }

  result.ok = true;
  result.record.remediation_job_id = meta->remediation_job_id;
  result.record.tenant_id = meta->tenant_id;
  result.record.vigil_incident_id = meta->vigil_incident_id;
  result.record.vigil_action_id = meta->vigil_action_id;
  result.record.request_id = meta->request_id;
  result.record.correlation_id = meta->correlation_id;
  result.record.chronos_job_id = meta->chronos_job_id;
  result.record.schedule_id = meta->schedule_id;
  result.record.execution_id = meta->execution_id;
  result.record.execution_state = cancel_requested
                                    ? std::string("CANCEL_REQUESTED")
                                    : serialization::ToString(execution->state);
  return result;
}

LookupVigilActionResult VigilRemediationService::GetByVigilActionId(
    const std::string& vigil_action_id,
    const std::string& tenant_id) const {
  LookupVigilActionResult result;

  std::optional<std::string> remediation_job_id;
  {
    std::lock_guard<std::mutex> lock(g_metadata_mu);
    const auto it = g_by_vigil_action_id.find(vigil_action_id);
    if (it != g_by_vigil_action_id.end()) {
      remediation_job_id = it->second;
    }
  }

  if (!remediation_job_id.has_value()) {
    result.ok = false;
    result.message = "vigil action not found";
    return result;
  }

  const auto lookup = GetRemediationJobById(*remediation_job_id, tenant_id);
  if (!lookup.ok) {
    result.ok = false;
    result.message = lookup.message;
    return result;
  }

  result.ok = true;
  result.record = lookup.record;
  return result;
}

CancelRemediationJobResult VigilRemediationService::CancelRemediationJob(
    const std::string& remediation_job_id,
    const std::string& tenant_id,
    const std::string& request_id,
    const std::string& correlation_id) const {
  (void)request_id;
  (void)correlation_id;

  CancelRemediationJobResult result;
  result.remediation_job_id = remediation_job_id;
  result.tenant_id = tenant_id;

  if (!context_ || !context_->execution_repository) {
    result.ok = false;
    result.message = "service dependencies unavailable";
    return result;
  }

  auto normalized_id = remediation_job_id;
  if (const auto colon = normalized_id.find(':'); colon != std::string::npos) {
    normalized_id = normalized_id.substr(0, colon);
  }
  if (const auto slash = normalized_id.find('/'); slash != std::string::npos) {
    normalized_id = normalized_id.substr(0, slash);
  }

  const auto lookup = GetRemediationJobById(normalized_id, tenant_id);
  if (!lookup.ok) {
    result.ok = false;
    result.message = lookup.message;
    return result;
  }

  result.remediation_job_id = lookup.record.remediation_job_id;

  const auto execution = context_->execution_repository->GetExecutionById(lookup.record.execution_id);
  if (!execution.has_value()) {
    result.ok = false;
    result.message = "execution not found";
    return result;
  }

  if (!IsCancellableExecutionState(execution->state)) {
    result.ok = false;
    result.message = "execution is in terminal or non-cancellable state";
    result.resulting_execution_state = serialization::ToString(execution->state);
    return result;
  }

  if (execution->state == domain::ExecutionState::kPending) {
    const auto transitioned = context_->execution_repository->TransitionExecutionState(
        execution->execution_id,
        domain::ExecutionState::kPending,
        domain::ExecutionState::kFailed,
        std::optional<std::string>("CANCELLED_BY_VIGIL"),
        std::optional<std::string>("cancel requested via integration endpoint"),
        std::optional<std::string>("api-integration"));

    if (!transitioned) {
      result.ok = false;
      result.message = "failed to cancel pending execution";
      return result;
    }

    result.ok = true;
    result.cancelled = true;
    result.message = "cancel accepted";
    result.resulting_execution_state = "FAILED";
    return result;
  }

  // DISPATCHED / RETRY_PENDING are not directly cancellable by current state machine.
  // Use state-guarded semantics: accepted but no immediate state mutation.
  {
    std::lock_guard<std::mutex> lock(g_metadata_mu);
    g_cancel_requested_by_remediation_job_id[lookup.record.remediation_job_id] = true;
  }

  result.ok = true;
  result.cancelled = false;
  result.message = "cancel accepted (deferred); execution state machine does not allow direct cancellation from current state";
  result.resulting_execution_state = "CANCEL_REQUESTED";
  return result;
}

}  // namespace chronos::api::application
