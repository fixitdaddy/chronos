# Phase 7: Redis Coordination & Protection + Phase 8 Observability Foundation (MVP)

This document records implementation delivered for Phase 7, plus baseline observability wiring expected for Phase 8 start.

## Phase 7 implemented scope

### 7.1 Distributed locks where DB constraints are not enough

Added Redis-coordination abstraction with lock semantics:

- `TryLock`
- `RefreshLock`
- `Unlock`

Used in scheduler dispatch flow for optional execution-window lock guard.

Files:

- `shared-core/include/chronos/coordination/redis_coordination.hpp`
- `shared-core/src/coordination/redis_coordination.cpp`
- `scheduler/src/core/scheduler_loop.cpp`

### 7.2 Rate limiting for API + per-job caps

Added Redis-backed (coordination-backed) limiters:

- global API method bucket in `ApiServer::Handle`
- per-job execution history endpoint cap in `HandleGetJobExecutions`

Fail-open when Redis unavailable to preserve correctness and availability.

Files:

- `api-server/src/http/server.cpp`
- `api-server/src/handlers/job_handlers.cpp`

### 7.3 Hot-path caching for job metadata

Added optional cache layer in `GET /jobs/:id`:

- read-through cache via coordination backend
- short TTL writes after DB read

File:

- `api-server/src/handlers/job_handlers.cpp`

### 7.4 Dedupe keys for dispatch/execution windows

Added coordination dedupe registration in scheduler dispatch flow:

- `dispatch:<schedule_id@time>` dedupe keys

Fail-open behavior if Redis unavailable (DB remains source of truth).

File:

- `scheduler/src/core/scheduler_loop.cpp`

### 7.5 Lock expiry/recovery under partition behavior

Added tests for:

- lock expiry and takeover
- temporary Redis unavailability behavior

Files:

- `tests/redis-coordination/redis_coordination_tests.cpp`
- `tests/redis-coordination/README.md`
- target: `chronos-redis-coordination-tests`

## Redis contract enforced

The implementation explicitly enforces the requested contract:

- Redis is optimization and coordination aid.
- Redis is **not** source of truth.
- Correctness remains grounded in durable DB and execution state transitions.
- On Redis unavailability:
  - rate-limit/dedupe paths fail-open,
  - lock paths fail-closed and DB constraints still protect correctness.

## Phase 8 observability foundation (implemented now)

While full Phase 8 work remains broader, this baseline is in place:

- scheduler election/scan/dispatch metrics (from Phase 6) already exposed via prometheus formatter
- API/worker/scheduler structured logging continues with stable JSON fields
- additional coordination paths now integrated without introducing silent failures

## Integration updates

- `HandlerContext` now includes `coordination` dependency.
- API main wires `InMemoryRedisCoordination` for local mode.
- scheduler loop and tests now accept coordination backend.
- shared-core build includes coordination module.

## Validation status

- Syntax checks pass (`g++ -fsyntax-only`) for shared-core, api-server, scheduler, worker, and new test harnesses.
- Full CMake build remains blocked until `cmake` is available in PATH.

## Exit criteria status (Phase 7 MVP)

- distributed locks (selective use): **done**
- API/per-job rate limits: **done**
- metadata hot cache: **done**
- dispatch dedupe keys: **done**
- partition/expiry recovery validation: **done**
- Redis unavailable correctness contract: **done**

## Known MVP limitations

1. Coordination backend is in-memory adapter; production Redis client adapter pending.
2. Rate limit policy is static and global; tenant-aware dynamic policies are next.
3. Cache invalidation is TTL-driven only; event-driven invalidation is next.
4. Dedupe windows are fixed; production should tune by schedule class and SLA.
