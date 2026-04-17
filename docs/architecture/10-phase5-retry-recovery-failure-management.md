# Phase 5: Retry, Recovery, and Failure Management (MVP)

This document records the implementation delivered for Phase 5.

## Implemented scope

### 5.1 Retry policies

Implemented and wired:

- fixed / linear / exponential backoff delay computation
- max-attempt enforcement
- retryable error classification (`retryable_error_codes_csv`)
- dead-letter move decision logic (`ShouldMoveToDeadLetter`)

Files:

- `shared-core/include/chronos/retry/backoff.hpp`
- `shared-core/src/retry/backoff.cpp`
- `worker/src/execution/result_handler.cpp`

### 5.2 Dead-letter handling + inspection endpoints

Added API endpoints:

- `GET /dead-letter/executions`
- `POST /dead-letter/executions/:execution_id/quarantine`

Files:

- `api-server/include/chronos/api/handlers/dead_letter_handlers.hpp`
- `api-server/src/handlers/dead_letter_handlers.cpp`
- `api-server/src/http/server.cpp`

### 5.3 Recovery scanner

Implemented recovery scanner handling:

- stale `RUNNING`
- lost worker heartbeat
- stuck `DISPATCHED`
- expired retries (`RETRY_PENDING`)

Repair actions:

- transition to `RETRY_PENDING`
- republish dispatch message to retry queue

Files:

- `shared-core/include/chronos/recovery/recovery_scanner.hpp`
- `shared-core/src/recovery/recovery_scanner.cpp`

### 5.4 Idempotency + deduplication

Implemented:

- in-memory idempotency lock store (Redis-lock analogue)
- execution idempotency key field in domain model
- in-memory repository uniqueness check on idempotency key
- worker runtime lock acquire/release around task execution

Files:

- `shared-core/include/chronos/idempotency/idempotency_store.hpp`
- `shared-core/src/idempotency/idempotency_store.cpp`
- `shared-core/include/chronos/domain/models.hpp`
- `shared-core/src/persistence/in_memory/in_memory_repositories.cpp`
- `worker/src/runtime/worker_runtime.cpp`

### 5.5 Poison job detection + quarantine

Implemented:

- poison-count tracking in execution model
- quarantine timestamp and reason fields
- poison detector with configurable threshold
- quarantine mutation API endpoint

Files:

- `worker/include/chronos/worker/recovery/poison_detector.hpp`
- `worker/src/recovery/poison_detector.cpp`
- `shared-core/include/chronos/domain/models.hpp`
- `shared-core/src/persistence/in_memory/in_memory_repositories.cpp`

### 5.6 Failure scenario suite

Implemented local failure scenario executable covering:

- duplicate delivery
- worker kill/recovery path
- DB reconnect simulation
- queue reconnect simulation

Files:

- `tests/failure-scenarios/failure_scenarios.cpp`
- `worker/CMakeLists.txt` target: `chronos-failure-scenarios`

## Additional repository support added

For local inspection and control:

- `GetAllExecutions()`
- `GetDeadLetterExecutions(limit, offset)`
- `MarkExecutionQuarantined(execution_id)`

File:

- `shared-core/include/chronos/persistence/in_memory/in_memory_repositories.hpp`
- `shared-core/src/persistence/in_memory/in_memory_repositories.cpp`

## Validation status

- Source syntax checks pass across `shared-core`, `api-server`, `scheduler`, `worker`, and `tests/failure-scenarios` using `g++ -fsyntax-only`.
- Full CMake build remains blocked until `cmake` is available in PATH.

## Exit criteria status (Phase 5 MVP)

- retry policies: **done**
- dead-letter handling + inspection endpoints: **done**
- recovery scanner: **done**
- idempotency and dedupe: **done (local in-memory implementation)**
- poison detection + quarantine: **done**
- failure scenario suite: **done (local executable)**

## Known MVP limitations

1. Idempotency lock store is in-memory; production should use Redis/DB fencing tokens.
2. Recovery scanner currently scans caller-provided candidate IDs; production should query DB windows by state/time.
3. Dead-letter APIs are fully implemented for local in-memory backend; production DB backend endpoint wiring remains pending.
4. Failure suite currently simulates reconnect behavior rather than driving real process/network disconnects.
