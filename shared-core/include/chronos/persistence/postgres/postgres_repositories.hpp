#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "chronos/domain/models.hpp"
#include "chronos/persistence/repository.hpp"

namespace chronos::persistence::postgres {

class PostgresConnection {
 public:
  explicit PostgresConnection(std::string connection_uri);

  [[nodiscard]] const std::string& connection_uri() const;

 private:
  std::string connection_uri_;
};

class PostgresTransaction final : public ITransaction {
 public:
  explicit PostgresTransaction(std::shared_ptr<PostgresConnection> connection);
  ~PostgresTransaction() override;

  bool Commit() override;
  void Rollback() override;

 private:
  std::shared_ptr<PostgresConnection> connection_;
  bool finished_{false};
};

class PostgresJobRepository final : public IJobRepository {
 public:
  explicit PostgresJobRepository(std::shared_ptr<PostgresConnection> connection);

  bool CreateJob(const domain::Job& job) override;
  std::optional<domain::Job> GetJobById(const std::string& job_id) override;
  bool UpdateJobState(const std::string& job_id, domain::JobState new_state) override;

 private:
  std::shared_ptr<PostgresConnection> connection_;
};

class PostgresScheduleRepository final : public IScheduleRepository {
 public:
  explicit PostgresScheduleRepository(std::shared_ptr<PostgresConnection> connection);

  bool UpsertSchedule(const domain::JobSchedule& schedule) override;
  std::vector<domain::JobSchedule> GetDueSchedules(
      domain::Timestamp as_of,
      std::size_t limit) override;

 private:
  std::shared_ptr<PostgresConnection> connection_;
};

class PostgresExecutionRepository final : public IExecutionRepository {
 public:
  PostgresExecutionRepository(
      std::shared_ptr<PostgresConnection> connection,
      std::shared_ptr<IAuditRepository> audit_repository,
      std::shared_ptr<IOutboxRepository> outbox_repository);

  bool CreateExecution(const domain::JobExecution& execution) override;
  std::optional<domain::JobExecution> GetExecutionById(const std::string& execution_id) override;
  bool TransitionExecutionState(
      const std::string& execution_id,
      domain::ExecutionState expected_from,
      domain::ExecutionState to,
      std::optional<std::string> error_code,
      std::optional<std::string> error_message,
      std::optional<std::string> worker_id) override;
  bool InsertAttempt(const domain::JobAttempt& attempt) override;
  bool UpdateAttemptState(
      const std::string& attempt_id,
      domain::AttemptState state,
      std::optional<std::string> error_code,
      std::optional<std::string> error_message) override;
  bool RecordWorkerHeartbeat(const domain::WorkerHeartbeat& heartbeat) override;
  std::vector<domain::JobExecution> GetExecutionHistoryForJob(
      const std::string& job_id,
      std::size_t limit,
      std::size_t offset) override;

 private:
  std::shared_ptr<PostgresConnection> connection_;
  std::shared_ptr<IAuditRepository> audit_repository_;
  std::shared_ptr<IOutboxRepository> outbox_repository_;
};

class PostgresAuditRepository final : public IAuditRepository {
 public:
  explicit PostgresAuditRepository(std::shared_ptr<PostgresConnection> connection);

  bool InsertExecutionEvent(
      const std::string& execution_id,
      const std::string& event_type,
      std::optional<domain::ExecutionState> from_state,
      std::optional<domain::ExecutionState> to_state,
      std::optional<std::string> error_code,
      std::optional<std::string> details_json) override;

 private:
  std::shared_ptr<PostgresConnection> connection_;
};

class PostgresOutboxRepository final : public IOutboxRepository {
 public:
  explicit PostgresOutboxRepository(std::shared_ptr<PostgresConnection> connection);

  bool InsertOutboxEvent(const domain::OutboxEvent& event) override;

 private:
  std::shared_ptr<PostgresConnection> connection_;
};

}  // namespace chronos::persistence::postgres
