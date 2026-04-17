# Phase 1: Core Domain and Persistence MVP (Implementation Notes)

This document records the concrete implementation choices for Phase 1.

## Scope implemented

### 1.1 Repo scaffold + shared libraries

Added `shared-core` structure with:

- `domain/` models and canonical enums
- `serialization/` enum codecs (string <-> enum)
- `time/` UTC clock helpers
- `retry/` backoff + dead-letter decision helpers
- `state_machine/` canonical execution transition validator
- `persistence/` repository interfaces
- `persistence/postgres/` PostgreSQL adapter skeleton and transactional transition entrypoint

### 1.2 PostgreSQL schema

Implemented SQL schema in `db-migrations/postgres/0001_init.sql` with:

- `jobs`
- `job_schedules`
- `job_executions`
- `job_attempts`
- `worker_heartbeats`
- `execution_events` (audit trail)
- `outbox_events` (optional, included)

### 1.3 Migrations + seed/dev tooling

Added:

- `db-migrations/postgres/0002_functions_and_triggers.sql`
- `db-migrations/postgres/0003_seed_dev.sql`
- `db-migrations/scripts/migrate.sh`
- `db-migrations/scripts/reset_dev.sh`
- `db-migrations/scripts/seed_dev.sh`

### 1.4 Repository layer (MVP interfaces + adapter skeleton)

Implemented repository interfaces in:

- `shared-core/include/chronos/persistence/repository.hpp`

Implemented PostgreSQL adapter skeleton in:

- `shared-core/include/chronos/persistence/postgres/postgres_repositories.hpp`
- `shared-core/src/persistence/postgres/postgres_repositories.cpp`

Includes methods for:

- job create/read/update
- execution insert/read/update transitions
- schedule upsert/due lookup
- attempts + heartbeats
- execution history queries
- audit and outbox insert points

### 1.5 Canonical state transitions + transaction safety

Implemented canonical transition gate:

- `PENDING -> DISPATCHED -> RUNNING -> SUCCEEDED/FAILED/RETRY_PENDING/DEAD_LETTER`
- `RETRY_PENDING -> DISPATCHED`

Implemented in:

- `shared-core/src/state_machine/execution_state_machine.cpp`

Repository transition method runs through transaction wrapper entrypoint (`PostgresTransaction`) to enforce atomic update + audit + outbox behavior once SQL wiring is completed.

### 1.6 Audit trail + execution history queries

Schema includes `execution_events` and indexes.
Repository interfaces + adapter stubs include:

- `InsertExecutionEvent`
- `GetExecutionHistoryForJob`

## What remains to reach production-ready Phase 1

1. Wire a concrete PostgreSQL C++ client (e.g. libpqxx) and replace TODO stubs with real SQL.
2. Add optimistic concurrency checks for transition updates (`WHERE execution_state = expected_from`) and rows-affected validation.
3. Add integration tests against real Postgres in CI.
4. Add id/trace generation utilities (UUID, request/trace IDs).
5. Extend retry metadata persistence in execution/attempt transitions.

## Exit criteria status

- Repo scaffold and shared domain libs: **done**
- Schema draft and migrations: **done**
- Seed/dev tooling: **done**
- Repository layer contracts: **done**
- Canonical transition guard: **done**
- Transactional persistence implementation: **partial** (scaffold + flow present, SQL client wiring pending)
- Audit/history plumbing: **partial** (schema + interfaces done, SQL wiring pending)
