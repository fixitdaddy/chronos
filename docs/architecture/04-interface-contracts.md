# Interface Contracts

This document defines the initial interface contracts for Chronos.

The goal of this phase is to make the external and cross-service shapes explicit
enough that implementation can begin without each module inventing its own
payload conventions.

## Contract rules

1. PostgreSQL is the durable source of truth; queue messages are projections of
   durable execution state.
2. REST contracts are control-plane interfaces exposed by `api-server`.
3. Queue contracts are internal runtime interfaces between `scheduler` and
   `worker`.
4. All timestamps use RFC 3339 / ISO 8601 strings at the API boundary and
   `TIMESTAMPTZ` in PostgreSQL.
5. All identifiers are UUIDs in public contracts unless noted otherwise.
6. Error responses are structured and machine-readable.

## REST API shape

Base path:

```text
/api/v1
```

Authentication:

- Token-based via `Authorization: Bearer <token>`

Content type:

- Request and response bodies use `application/json`

### `POST /jobs`

Creates a job and, optionally, its initial schedule.

#### Request body

```json
{
  "name": "daily-report",
  "task_type": "report.generate",
  "payload": {
    "report_id": "sales-summary"
  },
  "priority": 5,
  "queue_name": "default",
  "max_execution_time_seconds": 300,
  "idempotency_key": "client-request-123",
  "retry_policy": {
    "max_attempts": 5,
    "backoff_strategy": "EXPONENTIAL",
    "initial_delay_seconds": 30,
    "max_delay_seconds": 900,
    "backoff_multiplier": 2.0,
    "retry_on_timeout": true,
    "retry_on_worker_lost": true,
    "retryable_error_codes": [
      "NETWORK_ERROR",
      "RATE_LIMITED"
    ],
    "non_retryable_error_codes": [
      "INVALID_PAYLOAD"
    ]
  },
  "schedule": {
    "schedule_type": "CRON",
    "cron_expression": "0 2 * * *",
    "timezone": "UTC",
    "start_at": null,
    "end_at": null,
    "misfire_policy": "FIRE_ONCE"
  }
}
```

#### Validation rules

- `name`, `task_type`, and `queue_name` are required.
- `payload` must be valid JSON.
- `max_execution_time_seconds` must be positive.
- `schedule.schedule_type=CRON` requires `cron_expression`.
- `schedule.schedule_type=ONE_TIME` requires `run_at`.
- `retry_policy.max_attempts` must be at least `1`.

#### Response

- `201 Created` on success

```json
{
  "job_id": "d1c0e7d8-9d50-4f3a-88f4-5eaf1e6ea7b2",
  "job_state": "ACTIVE",
  "schedule_id": "2c7cd6cb-92ef-4eb8-96a1-0c6f8e1d779e",
  "created_at": "2026-04-17T19:30:00Z"
}
```

### `POST /jobs/{job_id}/schedules`

Adds or replaces a schedule for an existing job.

#### Request body

```json
{
  "schedule_type": "ONE_TIME",
  "run_at": "2026-04-18T08:00:00Z",
  "timezone": "UTC",
  "misfire_policy": "FIRE_ONCE"
}
```

#### Response

- `201 Created`

```json
{
  "schedule_id": "4507077b-0562-45c4-9cca-85cb2322ac22",
  "job_id": "d1c0e7d8-9d50-4f3a-88f4-5eaf1e6ea7b2",
  "next_run_at": "2026-04-18T08:00:00Z",
  "active": true
}
```

### `GET /jobs/{job_id}`

Returns the current job definition and active schedule summary.

#### Response

- `200 OK`

```json
{
  "job_id": "d1c0e7d8-9d50-4f3a-88f4-5eaf1e6ea7b2",
  "name": "daily-report",
  "task_type": "report.generate",
  "job_state": "ACTIVE",
  "queue_name": "default",
  "priority": 5,
  "max_execution_time_seconds": 300,
  "idempotency_key": "client-request-123",
  "created_at": "2026-04-17T19:30:00Z",
  "updated_at": "2026-04-17T19:30:00Z",
  "active_schedule": {
    "schedule_id": "2c7cd6cb-92ef-4eb8-96a1-0c6f8e1d779e",
    "schedule_type": "CRON",
    "cron_expression": "0 2 * * *",
    "timezone": "UTC",
    "next_run_at": "2026-04-18T02:00:00Z",
    "active": true
  }
}
```

### `GET /jobs/{job_id}/executions`

Returns execution history for a job.

#### Query parameters

- `limit`: default `50`, max `200`
- `cursor`: opaque pagination token

#### Response

- `200 OK`

```json
{
  "items": [
    {
      "execution_id": "2855c5b5-829c-4a03-8dd1-c6dc99d5dc2d",
      "execution_number": 12,
      "execution_state": "SUCCEEDED",
      "scheduled_at": "2026-04-17T02:00:00Z",
      "started_at": "2026-04-17T02:00:04Z",
      "finished_at": "2026-04-17T02:00:19Z",
      "attempt_count": 1,
      "current_worker_id": null,
      "last_error_code": null
    }
  ],
  "next_cursor": null
}
```

### `POST /jobs/{job_id}/pause`

Pauses future scheduling for the job.

#### Response

- `200 OK`

```json
{
  "job_id": "d1c0e7d8-9d50-4f3a-88f4-5eaf1e6ea7b2",
  "job_state": "PAUSED",
  "updated_at": "2026-04-17T19:40:00Z"
}
```

### `POST /jobs/{job_id}/resume`

Resumes future scheduling for the job.

### `POST /executions/{execution_id}/cancel`

Requests cancellation of a non-terminal execution.

### `GET /health`

Returns process health.

```json
{
  "status": "ok",
  "service": "api-server"
}
```

### `GET /metrics`

- Exposes Prometheus-compatible metrics.

## Queue message schema

Queue names:

- `main_queue`
- `retry_queue`
- `dead_letter_queue`

Delivery guarantee:

- At-least-once

Primary message type:

- `ExecutionDispatchMessage`

### Message body

```json
{
  "message_type": "execution.dispatch",
  "message_version": 1,
  "trace_id": "0a0ef8d0-e836-4d1c-b79f-0c4731f532e1",
  "job_id": "d1c0e7d8-9d50-4f3a-88f4-5eaf1e6ea7b2",
  "schedule_id": "2c7cd6cb-92ef-4eb8-96a1-0c6f8e1d779e",
  "execution_id": "2855c5b5-829c-4a03-8dd1-c6dc99d5dc2d",
  "execution_number": 12,
  "attempt": 1,
  "task_type": "report.generate",
  "queue_name": "default",
  "priority": 5,
  "scheduled_at": "2026-04-17T02:00:00Z",
  "dispatched_at": "2026-04-17T02:00:02Z",
  "max_execution_time_seconds": 300,
  "idempotency_key": "client-request-123",
  "payload": {
    "report_id": "sales-summary"
  }
}
```

### Message rules

- `execution_id` is the durable identity used for claims and result updates.
- `attempt` must match durable execution metadata in PostgreSQL.
- `message_version` is required for forward-compatible evolution.
- Workers must treat duplicate deliveries as valid and rely on durable state and
  lease checks before doing work.
- Queue headers may contain broker-specific delivery metadata, but business logic
  must not depend on headers alone.

## Database schema

The initial PostgreSQL schema is normalized around jobs, schedules, retry
policies, executions, and worker leases.

### `jobs`

Purpose:

- Stores the durable job definition.

Columns:

- `job_id UUID PRIMARY KEY`
- `name TEXT NOT NULL`
- `task_type TEXT NOT NULL`
- `payload JSONB NOT NULL`
- `priority INT NOT NULL DEFAULT 0`
- `queue_name TEXT NOT NULL`
- `max_execution_time_seconds INT NOT NULL`
- `retry_policy_id UUID NULL`
- `idempotency_key TEXT NULL`
- `job_state TEXT NOT NULL`
- `last_scheduled_at TIMESTAMPTZ NULL`
- `last_finished_at TIMESTAMPTZ NULL`
- `created_at TIMESTAMPTZ NOT NULL`
- `updated_at TIMESTAMPTZ NOT NULL`

Indexes:

- `(job_state)`
- `(task_type)`
- unique partial or scoped index on `idempotency_key` when product semantics
  require deduplication

### `retry_policies`

Purpose:

- Stores reusable retry definitions.

Columns:

- `retry_policy_id UUID PRIMARY KEY`
- `max_attempts INT NOT NULL`
- `backoff_strategy TEXT NOT NULL`
- `initial_delay_seconds INT NOT NULL`
- `max_delay_seconds INT NOT NULL`
- `backoff_multiplier DOUBLE PRECISION NOT NULL`
- `retry_on_timeout BOOLEAN NOT NULL`
- `retry_on_worker_lost BOOLEAN NOT NULL`
- `retryable_error_codes JSONB NULL`
- `non_retryable_error_codes JSONB NULL`
- `created_at TIMESTAMPTZ NOT NULL`
- `updated_at TIMESTAMPTZ NOT NULL`

### `schedules`

Purpose:

- Stores scheduling configuration attached to a job.

Columns:

- `schedule_id UUID PRIMARY KEY`
- `job_id UUID NOT NULL REFERENCES jobs(job_id)`
- `schedule_type TEXT NOT NULL`
- `cron_expression TEXT NULL`
- `run_at TIMESTAMPTZ NULL`
- `timezone TEXT NOT NULL`
- `start_at TIMESTAMPTZ NULL`
- `end_at TIMESTAMPTZ NULL`
- `next_run_at TIMESTAMPTZ NULL`
- `last_run_at TIMESTAMPTZ NULL`
- `misfire_policy TEXT NOT NULL`
- `active BOOLEAN NOT NULL DEFAULT TRUE`
- `created_at TIMESTAMPTZ NOT NULL`
- `updated_at TIMESTAMPTZ NOT NULL`

Indexes:

- `(next_run_at) WHERE active = TRUE`
- `(job_id, active)`

### `executions`

Purpose:

- Stores the durable lifecycle of each logical run.

Columns:

- `execution_id UUID PRIMARY KEY`
- `job_id UUID NOT NULL REFERENCES jobs(job_id)`
- `schedule_id UUID NULL REFERENCES schedules(schedule_id)`
- `execution_number BIGINT NOT NULL`
- `scheduled_at TIMESTAMPTZ NOT NULL`
- `dispatched_at TIMESTAMPTZ NULL`
- `started_at TIMESTAMPTZ NULL`
- `finished_at TIMESTAMPTZ NULL`
- `execution_state TEXT NOT NULL`
- `attempt_count INT NOT NULL DEFAULT 0`
- `max_attempts INT NOT NULL`
- `current_worker_id TEXT NULL`
- `last_heartbeat_at TIMESTAMPTZ NULL`
- `result_summary JSONB NULL`
- `last_error_code TEXT NULL`
- `last_error_message TEXT NULL`
- `created_at TIMESTAMPTZ NOT NULL`
- `updated_at TIMESTAMPTZ NOT NULL`

Indexes:

- `(job_id, execution_number DESC)`
- `(execution_state, scheduled_at)`
- `(current_worker_id)`
- `(last_heartbeat_at) WHERE execution_state = 'RUNNING'`

Constraints:

- unique `(job_id, execution_number)`

### `worker_leases`

Purpose:

- Stores active or recently expired worker ownership records for recovery and
  fencing.

Columns:

- `lease_id UUID PRIMARY KEY`
- `execution_id UUID NOT NULL REFERENCES executions(execution_id)`
- `worker_id TEXT NOT NULL`
- `lease_token UUID NOT NULL`
- `claimed_at TIMESTAMPTZ NOT NULL`
- `heartbeat_at TIMESTAMPTZ NOT NULL`
- `expires_at TIMESTAMPTZ NOT NULL`
- `status TEXT NOT NULL`
- `created_at TIMESTAMPTZ NOT NULL`
- `updated_at TIMESTAMPTZ NOT NULL`

Indexes:

- `(execution_id, status)`
- `(worker_id, status)`
- `(expires_at) WHERE status = 'ACTIVE'`

### `execution_events`

Purpose:

- Optional append-only audit log for lifecycle transitions.

Columns:

- `event_id UUID PRIMARY KEY`
- `execution_id UUID NOT NULL REFERENCES executions(execution_id)`
- `event_type TEXT NOT NULL`
- `from_state TEXT NULL`
- `to_state TEXT NULL`
- `error_code TEXT NULL`
- `details JSONB NULL`
- `created_at TIMESTAMPTZ NOT NULL`

## Config format

Chronos uses environment-first configuration with optional local file overrides
for development.

Recommended local file format:

- `YAML`

Rules:

- Environment variables override file values.
- Each service loads only its own section plus shared infrastructure config.
- Secrets should come from environment variables or secret managers, not
  committed files.

### Example config file

```yaml
service:
  name: api-server
  environment: development
  log_level: info

http:
  host: 0.0.0.0
  port: 8080
  request_timeout_seconds: 30

postgres:
  host: localhost
  port: 5432
  database: chronos
  user: chronos
  password_env: POSTGRES_PASSWORD
  max_connections: 20

rabbitmq:
  host: localhost
  port: 5672
  username: guest
  password_env: RABBITMQ_PASSWORD
  main_queue: main_queue
  retry_queue: retry_queue
  dead_letter_queue: dead_letter_queue

redis:
  host: localhost
  port: 6379

etcd:
  endpoints:
    - http://localhost:2379
  lease_ttl_seconds: 10

scheduler:
  polling_interval_ms: 1000
  batch_size: 100

worker:
  worker_id: worker-dev-1
  concurrency: 8
  heartbeat_interval_seconds: 5
  lease_ttl_seconds: 15
```

### Environment variable mapping

- `CHRONOS_SERVICE_NAME`
- `CHRONOS_ENV`
- `CHRONOS_LOG_LEVEL`
- `POSTGRES_HOST`
- `POSTGRES_PORT`
- `POSTGRES_DB`
- `POSTGRES_USER`
- `POSTGRES_PASSWORD`
- `RABBITMQ_HOST`
- `RABBITMQ_PORT`
- `RABBITMQ_USER`
- `RABBITMQ_PASSWORD`
- `REDIS_HOST`
- `REDIS_PORT`
- `ETCD_ENDPOINTS`

## Error model

All API errors use a common JSON envelope.

### Error response shape

```json
{
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "schedule.cron_expression is required for CRON schedules",
    "details": {
      "field": "schedule.cron_expression"
    },
    "retryable": false,
    "request_id": "b3812862-5933-4206-a3b9-d520761652c0"
  }
}
```

### Standard error codes

- `VALIDATION_ERROR`
- `UNAUTHORIZED`
- `FORBIDDEN`
- `NOT_FOUND`
- `CONFLICT`
- `RATE_LIMITED`
- `DEPENDENCY_UNAVAILABLE`
- `INTERNAL_ERROR`

### HTTP status mapping

- `VALIDATION_ERROR -> 400`
- `UNAUTHORIZED -> 401`
- `FORBIDDEN -> 403`
- `NOT_FOUND -> 404`
- `CONFLICT -> 409`
- `RATE_LIMITED -> 429`
- `DEPENDENCY_UNAVAILABLE -> 503`
- `INTERNAL_ERROR -> 500`

### Internal execution error codes

These codes are used inside execution metadata and retry evaluation:

- `TIMEOUT`
- `WORKER_LOST`
- `NETWORK_ERROR`
- `RATE_LIMITED`
- `INVALID_PAYLOAD`
- `TASK_HANDLER_NOT_FOUND`
- `TASK_EXECUTION_ERROR`
- `DEPENDENCY_FAILURE`

Rules:

- Internal execution error codes are not required to map one-to-one with public
  HTTP errors.
- Retry evaluation uses internal execution codes plus `RetryPolicy`.
- Every terminal failed or timed-out execution should store a stable error code.

## Acceptance criteria for interface lock

- API handlers have stable request and response shapes to implement.
- Queue publishers and consumers share one dispatch contract.
- Database migrations can be written from an agreed schema outline.
- Config loading has one canonical precedence model.
- Error handling is machine-readable and consistent across endpoints.
