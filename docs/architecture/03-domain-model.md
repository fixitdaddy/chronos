# Domain Model

This document defines the core entities and state enums for Chronos.

The goal of this phase is to lock the canonical nouns of the system before we
design schemas, APIs, or service-specific repositories. These definitions are
the shared language for `api-server`, `scheduler`, `worker`, and
`db-migrations`.

## Design rules

1. `Job` represents the durable definition of work.
2. `Schedule` represents when a job becomes eligible to run.
3. `Execution` represents one scheduled run of a job.
4. Retries create new attempts for the same execution lineage instead of new
   jobs.
5. Worker ownership is temporary and revocable; durable completion is stored in
   PostgreSQL.
6. State transitions must be explicit and monotonic unless a documented recovery
   path reopens work.

## Core entities

### `Job`

The durable business object created through the API.

#### Purpose

- Defines what should run.
- Holds execution configuration and routing metadata.
- Is the stable parent of schedules and executions.

#### Canonical fields

- `job_id`: globally unique identifier
- `name`: human-readable job name
- `task_type`: registered handler name that workers understand
- `payload`: task input payload
- `priority`: dispatch priority
- `queue_name`: target logical queue
- `max_execution_time`: timeout budget for a single attempt
- `retry_policy_id` or embedded retry policy reference
- `idempotency_key`: optional caller-provided deduplication key
- `created_at`
- `updated_at`
- `job_state`
- `last_scheduled_at`
- `last_finished_at`

#### Notes

- A `Job` is durable even if it has no active schedule.
- A one-time delayed job can be modeled as a job with a single active schedule.
- `task_type` is a contract key, not arbitrary code.

### `Schedule`

The timing definition attached to a job.

#### Purpose

- Decides when a job becomes due.
- Separates execution timing from the job definition itself.
- Allows the scheduler to compute the next due time deterministically.

#### Canonical fields

- `schedule_id`: globally unique identifier
- `job_id`: parent job reference
- `schedule_type`: `ONE_TIME` or `CRON`
- `cron_expression`: nullable, used for cron schedules
- `run_at`: nullable, used for one-time delayed jobs
- `timezone`: scheduling timezone for cron evaluation
- `start_at`: optional activation boundary
- `end_at`: optional expiry boundary
- `next_run_at`: next computed due timestamp
- `last_run_at`: last time the schedule produced an execution
- `misfire_policy`: how to handle overdue runs after downtime
- `active`: boolean lifecycle gate
- `created_at`
- `updated_at`

#### Notes

- A job may have one or more schedules if the product later chooses to allow
  that, but v1 should assume one active schedule per job to keep semantics
  simple.
- `next_run_at` is a derived but persisted field for efficient scheduling scans.

### `Execution`

The durable record for a specific scheduled run of a job.

#### Purpose

- Tracks a single logical run from dispatch through terminal completion.
- Owns attempt history and execution timing.
- Is the unit the worker claims and updates.

#### Canonical fields

- `execution_id`: globally unique identifier
- `job_id`: parent job reference
- `schedule_id`: nullable for ad hoc or immediate jobs
- `execution_number`: monotonic sequence per job
- `scheduled_at`: when the run was supposed to happen
- `dispatched_at`: when the scheduler published it
- `started_at`
- `finished_at`
- `execution_state`
- `attempt_count`
- `max_attempts`
- `current_worker_id`: nullable active owner
- `last_heartbeat_at`: nullable worker heartbeat
- `result_summary`: optional compact success/failure metadata
- `last_error_code`
- `last_error_message`
- `created_at`
- `updated_at`

#### Notes

- `Execution` is the durable identity used across queue messages and worker
  claims.
- Retries update the same execution lineage while increasing `attempt_count`.

### `RetryPolicy`

The policy that controls if and when an execution can be retried.

#### Purpose

- Encodes retry behavior independent of worker implementation details.
- Keeps retry decisions deterministic and inspectable.

#### Canonical fields

- `retry_policy_id`
- `max_attempts`
- `backoff_strategy`: `FIXED`, `LINEAR`, or `EXPONENTIAL`
- `initial_delay_seconds`
- `max_delay_seconds`
- `backoff_multiplier`
- `retry_on_timeout`: boolean
- `retry_on_worker_lost`: boolean
- `retryable_error_codes`: optional allowlist
- `non_retryable_error_codes`: optional denylist
- `created_at`
- `updated_at`

#### Notes

- Policy evaluation happens from durable metadata, not just queue headers or
  in-memory worker state.
- v1 can support `FIXED` and `EXPONENTIAL` first while keeping the enum open for
  future expansion.

### `WorkerLease`

The temporary ownership record that proves a worker is actively responsible for
an execution attempt.

#### Purpose

- Prevents multiple workers from actively processing the same execution attempt.
- Supports crash detection and recovery.

#### Canonical fields

- `lease_id`
- `execution_id`
- `worker_id`
- `lease_token`: unique fencing token for the claim
- `claimed_at`
- `heartbeat_at`
- `expires_at`
- `status`: `ACTIVE`, `EXPIRED`, `RELEASED`

#### Notes

- The lease may be persisted in PostgreSQL, Redis, or both depending on the
  implementation phase, but PostgreSQL remains the durable recovery source.
- A stale worker must not be able to finalize an execution after lease expiry if
  fencing is enforced correctly.

## State enums

### `JobState`

`JobState` describes the lifecycle of the durable job definition, not the status
of one particular run.

#### Values

- `ACTIVE`: eligible to be scheduled when its schedule becomes due
- `PAUSED`: retained but temporarily ineligible for new scheduling
- `COMPLETED`: finished permanently by business rule, typically after a one-time
  job succeeds and will not run again
- `FAILED`: disabled because the system or operator marked the job definition as
  unhealthy or invalid for further scheduling
- `DELETED`: tombstoned and hidden from active scheduling while retained for
  audit/history

#### Transition rules

- `ACTIVE -> PAUSED`
- `PAUSED -> ACTIVE`
- `ACTIVE -> COMPLETED`
- `ACTIVE -> FAILED`
- `PAUSED -> FAILED`
- `ACTIVE|PAUSED|COMPLETED|FAILED -> DELETED`

#### Notes

- `COMPLETED` is mainly relevant for one-time jobs.
- Repeated cron jobs typically remain `ACTIVE` even after successful executions.

### `ExecutionState`

`ExecutionState` describes the lifecycle of one logical run.

#### Values

- `PENDING`: execution record exists but has not yet been dispatched
- `DISPATCHED`: published to RabbitMQ and awaiting worker claim
- `RUNNING`: actively owned and executed by a worker
- `RETRY_PENDING`: last attempt failed but another attempt is scheduled
- `SUCCEEDED`: terminal success
- `FAILED`: terminal failure after retry policy exhaustion or non-retryable error
- `TIMED_OUT`: terminal timeout when retry is not allowed
- `CANCELLED`: cancelled by operator or control-plane decision before terminal
  success/failure
- `ABANDONED`: worker ownership was lost and the execution requires recovery
  handling before becoming runnable again

#### Transition rules

- `PENDING -> DISPATCHED`
- `DISPATCHED -> RUNNING`
- `RUNNING -> SUCCEEDED`
- `RUNNING -> FAILED`
- `RUNNING -> TIMED_OUT`
- `RUNNING -> RETRY_PENDING`
- `RUNNING -> ABANDONED`
- `DISPATCHED -> ABANDONED`
- `RETRY_PENDING -> DISPATCHED`
- `PENDING|DISPATCHED|RUNNING|RETRY_PENDING -> CANCELLED`
- `ABANDONED -> DISPATCHED`
- `ABANDONED -> FAILED`

#### Notes

- `ABANDONED` is a recovery state, not a final business result.
- `TIMED_OUT` is kept separate from `FAILED` because it matters operationally
  and for retry policy evaluation.

## State ownership by service

- `api-server` may create `Job` and `Schedule`, and may change `JobState` for
  control-plane actions such as pause, resume, cancel, or delete.
- `scheduler` creates `Execution` records and advances them into
  `PENDING` and `DISPATCHED`.
- `worker` advances executions through `RUNNING`, terminal states, and
  `RETRY_PENDING`.
- recovery logic in `scheduler` or a dedicated reconciler may move executions to
  or from `ABANDONED`.

## Derived behavior

### One-time job lifecycle

1. Create `Job` in `ACTIVE`.
2. Create `Schedule` with `schedule_type=ONE_TIME`.
3. Scheduler creates `Execution` in `PENDING`.
4. Execution flows to `SUCCEEDED` or another terminal state.
5. If successful and no further runs are expected, `JobState` can move to
   `COMPLETED`.

### Cron job lifecycle

1. Create `Job` in `ACTIVE`.
2. Create `Schedule` with `schedule_type=CRON`.
3. Each due time produces a new `Execution`.
4. Job remains `ACTIVE` across successful executions until paused, failed, or
   deleted.

### Retry lifecycle

1. Worker runs `Execution` attempt `n`.
2. Failure is evaluated against `RetryPolicy`.
3. If retryable and attempts remain, move to `RETRY_PENDING`.
4. Scheduler or retry dispatcher re-enqueues it and returns it to `DISPATCHED`.
5. If not retryable or attempts are exhausted, move to terminal state.

## Invariants

- A terminal execution state is one of:
  `SUCCEEDED`, `FAILED`, `TIMED_OUT`, `CANCELLED`.
- Only one active worker lease may exist for an execution attempt at a time.
- An execution cannot reach `RUNNING` without first being `DISPATCHED`.
- A job in `PAUSED` must not produce new executions.
- Deleting a job does not erase execution history.
- Retry count must never exceed the configured `max_attempts`.

## Acceptance criteria for domain lock

- Shared entities are defined clearly enough to design SQL tables next.
- State transitions have clear ownership and allowed paths.
- Recovery scenarios such as worker loss and retry are reflected in the model.
- API, scheduler, and worker teams can use the same vocabulary without
  ambiguity.
