# vigil ↔ chronos Phase 0.1 Inventory (Current State)

Date: 2026-04-18
Owner: integration baseline

## Scope
This document captures the actual current state before integration contract freeze:
1) chronos APIs, states, queue semantics, retry/dead-letter/recovery
2) vigil incident/action lifecycle and remediation trigger path

---

## A) chronos current state inventory

## A.1 API surface currently wired in code

From `api-server/src/http/server.cpp`:
- `POST /jobs`
- `POST /schedules`
- `GET /jobs/:id`
- `GET /jobs/:id/executions`
- `GET /health`
- `GET /metrics`
- `GET /dead-letter/executions`
- `POST /dead-letter/executions/:execution_id/quarantine` (limited/non-prod behavior noted)

Auth/tenant middleware behavior present in `server.cpp`:
- bearer auth check for non-health routes
- tenant header enforcement in strict mode

## A.2 Domain/state model implemented

Core enums in `shared-core/include/chronos/domain/models.hpp` and serialization in `shared-core/src/serialization/enum_codec.cpp`:

JobState:
- `ACTIVE`, `PAUSED`, `COMPLETED`, `FAILED`, `DELETED`

ExecutionState:
- `PENDING`, `DISPATCHED`, `RUNNING`, `SUCCEEDED`, `FAILED`, `RETRY_PENDING`, `DEAD_LETTER`

AttemptState:
- `CREATED`, `RUNNING`, `SUCCEEDED`, `FAILED`, `TIMED_OUT`, `CANCELLED`

State transition rules are centralized through repository/state-machine flow (`execution_state_machine`, `TransitionExecutionState`).

## A.3 Queue semantics and dispatch model

Queue names (design + implementation references):
- `main_queue`
- `retry_queue`
- `dead_letter_queue`

Execution flow (actual code paths):
- scheduler creates execution and transitions `PENDING -> DISPATCHED`
- worker claim transitions `DISPATCHED -> RUNNING`
- result handler transitions from RUNNING to terminal/retry states
- ack/nack behavior in consumer (`worker/src/messaging/rabbitmq_consumer.cpp`)

Durability direction:
- PostgreSQL is source of truth
- outbox table exists in migrations (`outbox_events`)

## A.4 Retry/dead-letter/recovery behavior

Retry/dead-letter:
- retry policy helpers in `shared-core/src/retry/backoff.cpp`
- result classification in `worker/src/execution/result_handler.cpp`
- terminal routing to `DEAD_LETTER` when thresholds exceeded

Recovery/scanning:
- `shared-core/src/recovery/recovery_scanner.cpp` handles stale/stuck cases
  - stale `RUNNING` -> `RETRY_PENDING`
  - stuck `DISPATCHED` handling
  - retry expiration handling

Poison handling:
- `worker/src/recovery/poison_detector.cpp`
- quarantine pathway is present but production-grade DB-backed endpoint wiring still partial per architecture docs

## A.5 Known implementation reality (important)

Production-minded architecture is present, but some internals remain scaffold/MVP level:
- queue client abstractions are strong, but some production AMQP specifics remain abstraction-heavy in parts
- dead-letter/quarantine API behavior has local-mode completeness; full production backend wiring is not fully closed in every path
- some phase docs explicitly note pending hardening details

---

## B) vigil current state inventory

## B.1 Current API surface (Python FastAPI)

Routers mounted from `python/app/main.py`:
- `POST /api/v1/ingest`
- `POST /api/v1/ingest/agent/metrics`
- `GET /api/v1/ingest/health`
- `POST /api/v1/actions`
- `GET /api/v1/actions`
- `GET /api/v1/actions/{id}`
- `GET /api/v1/actions/status/{status}`
- `PUT /api/v1/actions/{id}/cancel`
- `PUT /api/v1/actions/{id}/status`
- remediator compatibility:
  - `GET /remediator/tasks` (+ `/api/v1/actions/remediator/tasks`)
  - `POST /remediator/results` (+ `/api/v1/actions/remediator/results`)

Policy endpoints exist and are mounted when available (`/api/v1/policies*`).

## B.2 Action lifecycle currently implemented

Action statuses in `python/app/api/v1/actions.py`:
- `pending`, `running`, `completed`, `failed`, `cancelled`

Current patterns:
- action creation usually starts `pending`
- remediator task fetch marks selected actions `running`
- remediator result maps to `completed` or `failed`
- manual cancel path updates to `cancelled`

## B.3 Remediation trigger flow (current)

Current closed loop (Redis + worker + remediator):
1) metrics ingested (`/api/v1/ingest`)
2) policy engine evaluates conditions (`core/policy.py`)
3) actions can be created and optionally queued immediately
4) Redis queue (`core/queue.py`, list `remediation_queue`) stores tasks
5) worker (`services/worker.py`) dequeues and dispatches HTTP to remediator
6) remediator reports back via `/remediator/results`

Queue model in vigil is Redis LIST semantics (`RPUSH/BLPOP`), with queue stats and lightweight history tracking.

## B.4 Notable implementation inconsistencies/gaps

- `services/worker.py` updates action table via SQL using `WHERE action_id = :action_id`; model in actions API appears to use integer `id` column naming in ORM path.
- current vigil flow is internal Redis/remediator-centric; no direct chronos contract integration yet.
- no strict tenant-aware headers/identity propagation across action lifecycle by default.
- idempotency key semantics for remediation-triggered scheduling are not yet formalized as contracted behavior.

---

## C) Integration delta discovered in Phase 0.1

To move vigil→chronos cleanly, these are the key deltas from current state:

1. **No dedicated integration endpoints yet in chronos**
- need `/v1/integrations/vigil/*` add-on endpoints

2. **State model mismatch**
- vigil action statuses (pending/running/completed/failed/cancelled)
- chronos execution statuses (PENDING/DISPATCHED/RUNNING/SUCCEEDED/FAILED/RETRY_PENDING/DEAD_LETTER)
- requires explicit mapping contract + projector rules

3. **Idempotency missing cross-service contract**
- mandatory for remediation-triggered jobs
- needs `(tenant, endpoint, idempotency_key)` persistence + payload hash conflict detection

4. **Tenant/correlation propagation not standardized**
- requires strict headers + persistence fields + trace continuity

5. **Dual execution paths currently possible in vigil**
- internal Redis/remediator path exists
- future chronos dispatch path must be feature-flagged and migration-controlled

---

## D) Recommended immediate next step (Phase 0.2 input)

Freeze these decisions before coding integration:
- canonical headers and idempotency rules
- authoritative state mapping table
- migration path: coexist + canary + cutover
- which vigil pathways remain active during transition window

This inventory is the factual baseline for Phase 0.2 boundary freeze.
