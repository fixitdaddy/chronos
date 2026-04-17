# Concrete Defaults

This document locks the initial technology and environment defaults for
Chronos.

These defaults are chosen to reduce decision churn during the early phases of
implementation. Teams can revise them later through explicit architecture
changes, but the baseline for the first milestones is fixed here.

## Chosen defaults

### Language and build

- Language standard: `C++20`
- Build system: `CMake >= 3.28`
- Compiler targets:
  - `GCC 13+`
  - `Clang 17+`
- Build types:
  - `Debug`
  - `RelWithDebInfo`
  - `Release`

### Runtime dependencies

- Primary database: `PostgreSQL 16`
- Message broker: `RabbitMQ 3.13`
- Cache and coordination helper: `Redis 7`
- Leader election store: `etcd 3.5`

### Local development and deployment

- Local orchestration: `Docker Compose`
- Container image format: `Docker`
- Production orchestration target: `Kubernetes`

## Why these defaults

### `C++20`

- Gives us `std::jthread`, `std::stop_token`, `std::chrono` improvements, and
  better concurrency primitives for scheduler and worker runtimes.
- Is mature across modern GCC and Clang toolchains.

### `CMake`

- Works well for a multi-binary C++ repository.
- Integrates cleanly with IDEs, test runners, and container builds.
- Keeps dependency and target boundaries explicit.

### `PostgreSQL`

- Durable transactional system of record for jobs, schedules, and executions.
- Strong consistency for state transitions and recovery logic.
- Good support for indexing, time-window queries, and transactional updates.

### `RabbitMQ`

- Mature queueing semantics with acknowledgements, dead lettering, TTL, and
  retry routing.
- Good fit for at-least-once delivery and worker fan-out.

### `Redis`

- Fast fit for advisory locks, rate limiting, and bounded deduplication keys.
- Optional optimization layer without becoming the source of truth.

### `etcd`

- Strong fit for lease-based leader election and scheduler failover.
- Keeps leadership separate from application data.

### `Docker Compose` first, `Kubernetes` later

- Compose gives us a fast local feedback loop for multi-service development.
- Kubernetes is deferred until the service behavior is stable enough to deserve
  production deployment automation.

## Default repo conventions

### Build conventions

- Root `CMakeLists.txt` owns the workspace.
- Each module gets its own `CMakeLists.txt`.
- Shared code is compiled into reusable libraries under `shared-core`.
- `api-server`, `scheduler`, and `worker` build into separate executables.
- Tests are module-local and wired into `ctest`.

### Dependency strategy

- Prefer vendored or package-managed C++ dependencies with explicit version
  pinning.
- Minimize third-party surface in phase 1 and phase 2.
- Wrap infrastructure clients behind `shared-core` interfaces so service code is
  not tightly coupled to a single client library.

### Configuration strategy

- Environment variables are the outermost configuration source.
- Service-local config files can exist for development, but environment values
  must override file values.
- Each service receives only the configuration it needs.

### Environment naming

- `POSTGRES_*` for PostgreSQL settings
- `RABBITMQ_*` for RabbitMQ settings
- `REDIS_*` for Redis settings
- `ETCD_*` for etcd settings
- `CHRONOS_*` for service-specific settings

## Initial version assumptions

These version defaults are intentionally conservative and can be updated later:

| Component | Version |
| --- | --- |
| CMake | 3.28+ |
| GCC | 13+ |
| Clang | 17+ |
| PostgreSQL | 16 |
| RabbitMQ | 3.13 |
| Redis | 7 |
| etcd | 3.5 |

## Service runtime expectations

### `api-server`

- Stateless process.
- Connects to PostgreSQL directly.
- Does not require RabbitMQ, Redis, or etcd to accept simple reads, but create
  flows should fail fast if required downstream dependencies are unavailable.

### `scheduler`

- Requires PostgreSQL, RabbitMQ, and etcd.
- May use Redis for deduplication or rate-limited dispatch later, but Redis is
  not required in the first scheduling milestone.

### `worker`

- Requires RabbitMQ and PostgreSQL.
- May use Redis later for advisory execution protections.

## Local development topology

The default local stack in Docker Compose will include:

- `postgres`
- `rabbitmq`
- `redis`
- `etcd`
- optional future app services:
  - `api-server`
  - `scheduler`
  - `worker`

In early phases, Compose is allowed to run infrastructure only while binaries
are launched from the host for faster debugging.

## Deferred decisions

These are intentionally not locked in `0.2`:

- Exact HTTP framework
- Exact PostgreSQL client library
- Exact RabbitMQ client library
- Exact metrics and logging libraries
- Exact package manager or vendoring strategy for third-party C++ dependencies

Those choices belong in the next steps once core interfaces are defined.

## Acceptance criteria for default lock

- The repo has one canonical language and build baseline.
- Infrastructure choices are fixed for the first implementation milestones.
- Local development assumptions are explicit.
- Future module setup can proceed without revisiting platform basics.
