BEGIN;

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS jobs (
  job_id UUID PRIMARY KEY,
  name TEXT NOT NULL,
  task_type TEXT NOT NULL,
  payload JSONB NOT NULL,
  priority INT NOT NULL DEFAULT 0,
  queue_name TEXT NOT NULL,
  max_execution_time_seconds INT NOT NULL CHECK (max_execution_time_seconds > 0),
  retry_policy JSONB NOT NULL,
  idempotency_key TEXT NULL,
  job_state TEXT NOT NULL CHECK (job_state IN ('ACTIVE', 'PAUSED', 'COMPLETED', 'FAILED', 'DELETED')),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_jobs_state ON jobs (job_state);
CREATE INDEX IF NOT EXISTS idx_jobs_task_type ON jobs (task_type);
CREATE UNIQUE INDEX IF NOT EXISTS ux_jobs_idempotency_active
  ON jobs (idempotency_key)
  WHERE idempotency_key IS NOT NULL AND job_state <> 'DELETED';

CREATE TABLE IF NOT EXISTS job_schedules (
  schedule_id UUID PRIMARY KEY,
  job_id UUID NOT NULL REFERENCES jobs(job_id),
  schedule_type TEXT NOT NULL CHECK (schedule_type IN ('ONE_TIME', 'CRON')),
  cron_expression TEXT NULL,
  run_at TIMESTAMPTZ NULL,
  timezone TEXT NOT NULL,
  start_at TIMESTAMPTZ NULL,
  end_at TIMESTAMPTZ NULL,
  next_run_at TIMESTAMPTZ NULL,
  last_run_at TIMESTAMPTZ NULL,
  misfire_policy TEXT NOT NULL,
  active BOOLEAN NOT NULL DEFAULT TRUE,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT chk_schedule_shape CHECK (
    (schedule_type = 'CRON' AND cron_expression IS NOT NULL AND run_at IS NULL)
    OR
    (schedule_type = 'ONE_TIME' AND run_at IS NOT NULL AND cron_expression IS NULL)
  )
);

CREATE INDEX IF NOT EXISTS idx_job_schedules_due ON job_schedules (next_run_at)
  WHERE active = TRUE;
CREATE INDEX IF NOT EXISTS idx_job_schedules_job_active ON job_schedules (job_id, active);

CREATE TABLE IF NOT EXISTS job_executions (
  execution_id UUID PRIMARY KEY,
  job_id UUID NOT NULL REFERENCES jobs(job_id),
  schedule_id UUID NULL REFERENCES job_schedules(schedule_id),
  execution_number BIGINT NOT NULL,
  scheduled_at TIMESTAMPTZ NOT NULL,
  dispatched_at TIMESTAMPTZ NULL,
  started_at TIMESTAMPTZ NULL,
  finished_at TIMESTAMPTZ NULL,
  execution_state TEXT NOT NULL CHECK (execution_state IN (
    'PENDING', 'DISPATCHED', 'RUNNING', 'SUCCEEDED', 'FAILED', 'RETRY_PENDING', 'DEAD_LETTER'
  )),
  attempt_count INT NOT NULL DEFAULT 0,
  max_attempts INT NOT NULL CHECK (max_attempts >= 1),
  current_worker_id TEXT NULL,
  last_heartbeat_at TIMESTAMPTZ NULL,
  result_summary JSONB NULL,
  last_error_code TEXT NULL,
  last_error_message TEXT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT ux_job_execution_number UNIQUE (job_id, execution_number)
);

CREATE INDEX IF NOT EXISTS idx_job_executions_state_time ON job_executions (execution_state, scheduled_at);
CREATE INDEX IF NOT EXISTS idx_job_executions_job_number ON job_executions (job_id, execution_number DESC);
CREATE INDEX IF NOT EXISTS idx_job_executions_worker ON job_executions (current_worker_id);
CREATE INDEX IF NOT EXISTS idx_job_executions_heartbeat_running ON job_executions (last_heartbeat_at)
  WHERE execution_state = 'RUNNING';

CREATE TABLE IF NOT EXISTS job_attempts (
  attempt_id UUID PRIMARY KEY,
  execution_id UUID NOT NULL REFERENCES job_executions(execution_id),
  attempt_number INT NOT NULL CHECK (attempt_number >= 1),
  attempt_state TEXT NOT NULL CHECK (attempt_state IN ('CREATED', 'RUNNING', 'SUCCEEDED', 'FAILED', 'TIMED_OUT', 'CANCELLED')),
  worker_id TEXT NULL,
  started_at TIMESTAMPTZ NULL,
  finished_at TIMESTAMPTZ NULL,
  error_code TEXT NULL,
  error_message TEXT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT ux_attempt_execution_number UNIQUE (execution_id, attempt_number)
);

CREATE INDEX IF NOT EXISTS idx_job_attempts_execution ON job_attempts (execution_id, attempt_number DESC);
CREATE INDEX IF NOT EXISTS idx_job_attempts_worker ON job_attempts (worker_id);
CREATE INDEX IF NOT EXISTS idx_job_attempts_state ON job_attempts (attempt_state);

CREATE TABLE IF NOT EXISTS worker_heartbeats (
  heartbeat_id UUID PRIMARY KEY,
  worker_id TEXT NOT NULL,
  execution_id UUID NOT NULL REFERENCES job_executions(execution_id),
  attempt_number INT NOT NULL,
  heartbeat_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  details JSONB NULL
);

CREATE INDEX IF NOT EXISTS idx_worker_heartbeats_execution_time ON worker_heartbeats (execution_id, heartbeat_at DESC);
CREATE INDEX IF NOT EXISTS idx_worker_heartbeats_worker_time ON worker_heartbeats (worker_id, heartbeat_at DESC);

CREATE TABLE IF NOT EXISTS execution_events (
  event_id UUID PRIMARY KEY,
  execution_id UUID NOT NULL REFERENCES job_executions(execution_id),
  event_type TEXT NOT NULL,
  from_state TEXT NULL,
  to_state TEXT NULL,
  error_code TEXT NULL,
  details JSONB NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_execution_events_execution_time ON execution_events (execution_id, created_at DESC);
CREATE INDEX IF NOT EXISTS idx_execution_events_type_time ON execution_events (event_type, created_at DESC);

CREATE TABLE IF NOT EXISTS outbox_events (
  event_id UUID PRIMARY KEY,
  aggregate_type TEXT NOT NULL,
  aggregate_id TEXT NOT NULL,
  event_type TEXT NOT NULL,
  payload JSONB NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  published_at TIMESTAMPTZ NULL,
  publish_attempts INT NOT NULL DEFAULT 0,
  lock_token UUID NULL
);

CREATE INDEX IF NOT EXISTS idx_outbox_events_unpublished ON outbox_events (created_at)
  WHERE published_at IS NULL;

COMMIT;
