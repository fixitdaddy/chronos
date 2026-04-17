#pragma once

#include <optional>
#include <string>
#include <vector>

#include "chronos/domain/models.hpp"

namespace chronos::persistence {

class ITransaction {
 public:
  virtual ~ITransaction() = default;
  virtual bool Commit() = 0;
  virtual void Rollback() = 0;
};

class IJobRepository {
 public:
  virtual ~IJobRepository() = default;
  virtual bool CreateJob(const domain::Job& job) = 0;
  virtual std::optional<domain::Job> GetJobById(const std::string& job_id) = 0;
  virtual bool UpdateJobState(
      const std::string& job_id,
      domain::JobState new_state) = 0;
};

class IScheduleRepository {
 public:
  virtual ~IScheduleRepository() = default;
  virtual bool UpsertSchedule(const domain::JobSchedule& schedule) = 0;
  virtual std::vector<domain::JobSchedule> GetDueSchedules(
      domain::Timestamp as_of,
      std::size_t limit) = 0;
};

class IExecutionRepository {
 public:
  virtual ~IExecutionRepository() = default;
  virtual bool CreateExecution(const domain::JobExecution& execution) = 0;
  virtual std::optional<domain::JobExecution> GetExecutionById(
      const std::string& execution_id) = 0;

  virtual bool TransitionExecutionState(
      const std::string& execution_id,
      domain::ExecutionState expected_from,
      domain::ExecutionState to,
      std::optional<std::string> error_code,
      std::optional<std::string> error_message,
      std::optional<std::string> worker_id) = 0;

  virtual bool InsertAttempt(const domain::JobAttempt& attempt) = 0;
  virtual bool UpdateAttemptState(
      const std::string& attempt_id,
      domain::AttemptState state,
      std::optional<std::string> error_code,
      std::optional<std::string> error_message) = 0;

  virtual bool RecordWorkerHeartbeat(const domain::WorkerHeartbeat& heartbeat) = 0;

  virtual std::vector<domain::JobExecution> GetExecutionHistoryForJob(
      const std::string& job_id,
      std::size_t limit,
      std::size_t offset) = 0;
};

class IAuditRepository {
 public:
  virtual ~IAuditRepository() = default;
  virtual bool InsertExecutionEvent(
      const std::string& execution_id,
      const std::string& event_type,
      std::optional<domain::ExecutionState> from_state,
      std::optional<domain::ExecutionState> to_state,
      std::optional<std::string> error_code,
      std::optional<std::string> details_json) = 0;
};

class IOutboxRepository {
 public:
  virtual ~IOutboxRepository() = default;
  virtual bool InsertOutboxEvent(const domain::OutboxEvent& event) = 0;
};

}  // namespace chronos::persistence
