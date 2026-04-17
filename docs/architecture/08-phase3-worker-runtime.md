# Phase 3: Worker Runtime and Async Execution (Local Mode)

This document records Phase 3 implementation status.

## Implemented scope

### 3.1 Worker process with thread pool, task adapter interface, graceful shutdown hooks

Implemented:

- `ThreadPool` with queue depth and active worker counters
- async submission API
- graceful shutdown with configurable drain behavior
- worker runtime executable (`chronos-worker`)

Files:

- `worker/src/runtime/thread_pool.cpp`
- `worker/src/runtime/worker_runtime.cpp`
- `worker/src/main.cpp`

### 3.2 Task execution contract

Defined canonical task contract including:

- input payload (`payload_json`)
- timeout budget
- cancellation token
- result metadata
- idempotency key

Task result types:

- `SUCCESS`
- `RETRYABLE_FAILURE`
- `PERMANENT_FAILURE`
- `TIMEOUT`
- `ABANDONED`

Files:

- `worker/include/chronos/worker/task/task_contract.hpp`
- `worker/src/task/task_contract.cpp`
- `worker/include/chronos/worker/task/task_registry.hpp`
- `worker/src/task/task_registry.cpp`

### 3.3 Execution claims (single active owner)

Implemented claim path via transactional state transition:

- claim uses `DISPATCHED -> RUNNING`
- if transition fails, worker does not execute

File:

- `worker/src/execution/claim_manager.cpp`

### 3.4 Timeout detection, heartbeat updates, shutdown drain behavior

Implemented:

- task timeout detection via `future.wait_for(timeout)`
- periodic heartbeat writer while task is running
- thread-pool drain vs immediate-stop behavior on shutdown

Files:

- `worker/src/execution/heartbeat_manager.cpp`
- `worker/src/runtime/worker_runtime.cpp`
- `worker/src/runtime/thread_pool.cpp`

### 3.5 Task result handling

Implemented mapping from task result kinds to execution states:

- success -> `SUCCEEDED`
- retryable failure -> `RETRY_PENDING` (or `DEAD_LETTER` when max attempts reached)
- permanent failure -> `FAILED`
- timeout -> `RETRY_PENDING` with `TIMEOUT`
- abandoned -> `RETRY_PENDING` with `ABANDONED`

File:

- `worker/src/execution/result_handler.cpp`

### 3.6 Local stress testing for concurrency and saturation

Implemented stress harness executable `chronos-worker-stress`:

- submits 500 dispatched executions
- runs handler under bounded thread pool
- reports completion count and exits non-zero on mismatch

File:

- `worker/src/stress/local_stress.cpp`

## Observability

Added worker structured logger helper:

- level, worker_id, message, extra

File:

- `worker/src/observability/logger.cpp`

## Build wiring

- Replaced placeholder `worker/CMakeLists.txt` with real targets:
  - `chronos-worker-lib`
  - `chronos-worker`
  - `chronos-worker-stress`

## Validation status

- All worker `.cpp` units pass syntax checks using `g++ -fsyntax-only`.
- Full CMake configure/build remains blocked until `cmake` is installed in PATH.

## Exit criteria status (Phase 3 MVP)

- Worker runtime + thread pool + graceful shutdown: **done**
- Task execution contract: **done**
- Execution claims: **done**
- Timeout + heartbeat + drain: **done**
- Result handling states: **done**
- Local stress harness: **done**

## Known MVP limitations

1. Current runtime ingests execution IDs through local submit API (no RabbitMQ consumer yet).
2. Attempt numbering is fixed at `1` in current runtime path.
3. Cancellation token is defined but not externally signaled yet.
4. Claim/heartbeat durability semantics inherit repository backend quality (in-memory for local mode; PostgreSQL wiring pending from Phase 1 hardening).
