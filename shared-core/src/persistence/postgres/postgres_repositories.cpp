#include "chronos/persistence/postgres/postgres_repositories.hpp"

#include <iostream>
#include <utility>

#include "chronos/retry/backoff.hpp"
#include "chronos/serialization/enum_codec.hpp"
#include "chronos/state_machine/execution_state_machine.hpp"

namespace chronos::persistence::postgres {

PostgresConnection::PostgresConnection(std::string connection_uri)
    : connection_uri_(std::move(connection_uri)) {}

const std::string& PostgresConnection::connection_uri() const {
  return connection_uri_;
}

PostgresTransaction::PostgresTransaction(
    std::shared_ptr<PostgresConnection> connection)
    : connection_(std::move(connection)) {
  (void)connection_;
  // TODO: BEGIN transaction in real PG adapter.
}

PostgresTransaction::~PostgresTransaction() {
  if (!finished_) {
    Rollback();
  }
}

bool PostgresTransaction::Commit() {
  finished_ = true;
  // TODO: COMMIT transaction.
  return true;
}

void PostgresTransaction::Rollback() {
  finished_ = true;
  // TODO: ROLLBACK transaction.
}

PostgresJobRepository::PostgresJobRepository(
    std::shared_ptr<PostgresConnection> connection)
    : connection_(std::move(connection)) {}

bool PostgresJobRepository::CreateJob(const domain::Job& job) {
  (void)connection_;
  (void)job;
  // TODO: INSERT INTO jobs (...)
  return true;
}

std::optional<domain::Job> PostgresJobRepository::GetJobById(
    const std::string& job_id) {
  (void)connection_;
  (void)job_id;
  // TODO: SELECT * FROM jobs WHERE job_id = $1
  return std::nullopt;
}

bool PostgresJobRepository::UpdateJobState(
    const std::string& job_id,
    domain::JobState new_state) {
  (void)connection_;
  (void)job_id;
  (void)new_state;
  // TODO: UPDATE jobs SET job_state = $2, updated_at = NOW() WHERE job_id = $1
  return true;
}

PostgresScheduleRepository::PostgresScheduleRepository(
    std::shared_ptr<PostgresConnection> connection)
    : connection_(std::move(connection)) {}

bool PostgresScheduleRepository::UpsertSchedule(const domain::JobSchedule& schedule) {
  (void)connection_;
  (void)schedule;
  // TODO: UPSERT job_schedules by schedule_id.
  return true;
}

std::vector<domain::JobSchedule> PostgresScheduleRepository::GetDueSchedules(
    domain::Timestamp as_of,
    std::size_t limit) {
  (void)connection_;
  (void)as_of;
  (void)limit;
  // TODO: SELECT active schedules WHERE next_run_at <= as_of.
  return {};
}

PostgresExecutionRepository::PostgresExecutionRepository(
    std::shared_ptr<PostgresConnection> connection,
    std::shared_ptr<IAuditRepository> audit_repository,
    std::shared_ptr<IOutboxRepository> outbox_repository)
    : connection_(std::move(connection)),
      audit_repository_(std::move(audit_repository)),
      outbox_repository_(std::move(outbox_repository)) {}

bool PostgresExecutionRepository::CreateExecution(const domain::JobExecution& execution) {
  (void)connection_;
  (void)execution;
  // TODO: INSERT execution + maybe initial attempt row in one transaction.
  return true;
}

std::optional<domain::JobExecution> PostgresExecutionRepository::GetExecutionById(
    const std::string& execution_id) {
  (void)connection_;
  (void)execution_id;
  // TODO: SELECT * FROM job_executions WHERE execution_id = $1
  return std::nullopt;
}

bool PostgresExecutionRepository::TransitionExecutionState(
    const std::string& execution_id,
    domain::ExecutionState expected_from,
    domain::ExecutionState to,
    std::optional<std::string> error_code,
    std::optional<std::string> error_message,
    std::optional<std::string> worker_id) {
  (void)connection_;

  const auto decision = state_machine::ValidateExecutionTransition(expected_from, to);
  if (!decision.allowed) {
    return false;
  }

  PostgresTransaction txn(connection_);

  // TODO: UPDATE job_executions
  //   SET execution_state = :to,
  //       last_error_code = :error_code,
  //       last_error_message = :error_message,
  //       current_worker_id = :worker_id,
  //       updated_at = NOW(),
  //       started_at = CASE WHEN :to='RUNNING' THEN NOW() ELSE started_at END,
  //       finished_at = CASE WHEN :to IN ('SUCCEEDED','FAILED','DEAD_LETTER') THEN NOW() ELSE finished_at END,
  //       attempt_count = CASE WHEN :to='RUNNING' THEN attempt_count + 1 ELSE attempt_count END
  //   WHERE execution_id = :execution_id AND execution_state = :expected_from

  if (audit_repository_) {
    audit_repository_->InsertExecutionEvent(
        execution_id,
        "execution.state_transition",
        expected_from,
        to,
        error_code,
        std::nullopt);
  }

  if (outbox_repository_) {
    domain::OutboxEvent event{
        .event_id = "TODO-generate-uuid",
        .aggregate_type = "job_execution",
        .aggregate_id = execution_id,
        .event_type = "execution." + serialization::ToString(to),
        .payload_json = "{}",
    };
    outbox_repository_->InsertOutboxEvent(event);
  }

  return txn.Commit();
}

bool PostgresExecutionRepository::InsertAttempt(const domain::JobAttempt& attempt) {
  (void)connection_;
  (void)attempt;
  // TODO: INSERT INTO job_attempts (...)
  return true;
}

bool PostgresExecutionRepository::UpdateAttemptState(
    const std::string& attempt_id,
    domain::AttemptState state,
    std::optional<std::string> error_code,
    std::optional<std::string> error_message) {
  (void)connection_;
  (void)attempt_id;
  (void)state;
  (void)error_code;
  (void)error_message;
  // TODO: UPDATE job_attempts SET state, error fields, finished_at
  return true;
}

bool PostgresExecutionRepository::RecordWorkerHeartbeat(
    const domain::WorkerHeartbeat& heartbeat) {
  (void)connection_;
  (void)heartbeat;
  // TODO: INSERT heartbeat and update job_executions.last_heartbeat_at
  return true;
}

std::vector<domain::JobExecution> PostgresExecutionRepository::GetExecutionHistoryForJob(
    const std::string& job_id,
    std::size_t limit,
    std::size_t offset) {
  (void)connection_;
  (void)job_id;
  (void)limit;
  (void)offset;
  // TODO: SELECT executions ORDER BY execution_number DESC LIMIT/OFFSET
  return {};
}

PostgresAuditRepository::PostgresAuditRepository(
    std::shared_ptr<PostgresConnection> connection)
    : connection_(std::move(connection)) {}

bool PostgresAuditRepository::InsertExecutionEvent(
    const std::string& execution_id,
    const std::string& event_type,
    std::optional<domain::ExecutionState> from_state,
    std::optional<domain::ExecutionState> to_state,
    std::optional<std::string> error_code,
    std::optional<std::string> details_json) {
  (void)connection_;
  (void)execution_id;
  (void)event_type;
  (void)from_state;
  (void)to_state;
  (void)error_code;
  (void)details_json;
  // TODO: INSERT INTO execution_events (...)
  return true;
}

PostgresOutboxRepository::PostgresOutboxRepository(
    std::shared_ptr<PostgresConnection> connection)
    : connection_(std::move(connection)) {}

bool PostgresOutboxRepository::InsertOutboxEvent(const domain::OutboxEvent& event) {
  (void)connection_;
  (void)event;
  // TODO: INSERT INTO outbox_events (...)
  return true;
}

}  // namespace chronos::persistence::postgres
