# System Boundaries

This document finalizes the high-level boundaries for the first Chronos
architecture milestone.

The goal is to separate control-plane concerns from execution concerns early, so
we can scale each independently and keep recovery logic deterministic.

## Design rules

1. PostgreSQL is the source of truth for jobs, schedules, executions, and
   recovery state.
2. RabbitMQ is a delivery mechanism, not the system of record.
3. Redis is advisory infrastructure for deduplication, locks, rate limits, and
   cacheable hot paths.
4. etcd is used only for scheduler leadership and related coordination.
5. Workers are stateless with respect to durable job metadata.
6. Shared code must contain reusable primitives and contracts, not service-owned
   business workflows.

## Modules

### `api-server`

#### Responsibilities

- Accept job creation, schedule creation, and job query requests.
- Authenticate and authorize incoming requests.
- Validate request payloads and translate them into domain commands.
- Persist jobs and schedules through shared persistence interfaces.
- Return job status, execution history, and health information.

#### Owns

- HTTP API surface
- Request/response DTO mapping
- Authentication middleware
- Input validation rules
- Read/query endpoints

#### Must not own

- Scheduling loop logic
- Queue consumption
- Job execution logic
- Leader election

#### Depends on

- `shared-core`

### `scheduler`

#### Responsibilities

- Participate in leader election using etcd.
- Continuously discover due jobs from PostgreSQL.
- Materialize execution attempts for due jobs.
- Publish execution messages to RabbitMQ.
- Reconcile stuck or expired scheduled work when required by recovery policy.

#### Owns

- Scheduling loop
- Cron and delayed job trigger evaluation
- Dispatch batching
- Leader lifecycle
- Scheduler recovery scanners

#### Must not own

- HTTP request handling
- Task handler execution
- Worker thread pool logic

#### Depends on

- `shared-core`

### `worker`

#### Responsibilities

- Consume execution messages from RabbitMQ.
- Claim and execute job attempts safely.
- Heartbeat long-running executions.
- Apply timeout, retry, and terminal-failure decisions.
- Persist execution results and lifecycle updates.

#### Owns

- Queue consumer runtime
- Task execution adapters
- Worker thread pool
- Graceful shutdown and drain behavior
- Execution heartbeats

#### Must not own

- Scheduling decisions
- Public API endpoints
- Global cluster leadership

#### Depends on

- `shared-core`

### `shared-core`

#### Responsibilities

- Define shared domain models and enums.
- Define service-to-service contracts and message schemas.
- Provide persistence abstractions and shared infrastructure adapters.
- Provide time, serialization, config, logging, metrics, retry, and error
  primitives used by multiple services.

#### Owns

- Canonical job and execution state models
- Queue message schema
- Config schema and loader contracts
- DB repository interfaces
- Shared PostgreSQL, RabbitMQ, Redis, and etcd client wrappers
- Observability helpers

#### Must not own

- Service orchestration loops
- HTTP routing
- Service-specific deployment scripts

### `db-migrations`

#### Responsibilities

- Version and apply PostgreSQL schema changes.
- Define initial bootstrap schema and seed/dev helpers where needed.
- Provide rollback or compatibility notes for schema changes.

#### Owns

- Migration SQL files
- Schema bootstrap scripts
- Database version manifest

#### Must not own

- Runtime business logic
- Application-side repositories

### `ops`

#### Responsibilities

- Local development environment definition.
- Container build definitions.
- Deployment manifests for Kubernetes.
- Observability bootstrap for Prometheus, Grafana, and logging stack.
- Runbooks and operational scripts.

#### Owns

- Dockerfiles and compose files
- Kubernetes manifests or Helm charts
- Dashboards and alerts
- Environment examples
- Operational docs

#### Must not own

- Core business logic
- Runtime state transitions

## Dependency direction

Allowed dependency graph:

```text
api-server -> shared-core
scheduler  -> shared-core
worker     -> shared-core
db-migrations -> no runtime module dependency
ops -> may reference all modules for build/deploy wiring only
```

Forbidden dependencies:

```text
api-server -> scheduler
api-server -> worker
scheduler  -> api-server
worker     -> api-server
worker     -> scheduler
shared-core -> api-server/scheduler/worker/ops
```

## Data ownership

- `jobs` and `job_schedules` are created through `api-server`.
- `job_executions` and `job_attempts` are created by `scheduler` and updated by
  `worker`.
- Scheduler leadership metadata lives in etcd, not PostgreSQL.
- Delivery state in RabbitMQ is transient and must always be recoverable from
  PostgreSQL state.

## Initial integration contracts

### API to persistence

- The API layer issues domain commands such as `CreateJob`, `CreateSchedule`,
  and `GetJobStatus`.
- Persistence commits must complete before the API returns a success response.

### Scheduler to queue

- The scheduler creates a durable execution record in PostgreSQL before
  publishing a message.
- Queue publication includes stable identifiers such as `job_id`,
  `execution_id`, `attempt`, `scheduled_at`, and `trace_id`.

### Worker to persistence

- A worker updates execution state in PostgreSQL before acknowledging terminal
  completion.
- Retry eligibility is decided from durable execution metadata, not only from
  queue headers.

## Suggested top-level layout

```text
chronos/
  api-server/
    include/
    src/
    tests/
  scheduler/
    include/
    src/
    tests/
  worker/
    include/
    src/
    tests/
  shared-core/
    include/
    src/
    tests/
  db-migrations/
    postgres/
  ops/
    docker/
    compose/
    kubernetes/
    observability/
  docs/
    architecture/
```

## Acceptance criteria for boundary lock

- Each service has a single, non-overlapping responsibility.
- Durable state ownership is unambiguous.
- Shared code is reusable without pulling in service-specific behavior.
- Future contributors can tell where a new feature belongs before coding.
