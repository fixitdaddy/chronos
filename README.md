# Chronos

Chronos is a distributed job scheduling and execution system in C++.

This repository starts with the system architecture and module boundaries so
implementation can proceed without service responsibilities drifting over time.

## Concrete defaults

- Language standard: `C++20`
- Build system: `CMake`
- Source of truth database: `PostgreSQL`
- Message broker: `RabbitMQ`
- Advisory cache and locks: `Redis`
- Scheduler leader election: `etcd`
- Local development platform: `Docker Compose`
- Future deployment target: `Kubernetes`

## Initial module layout

- `api-server/` accepts job and schedule requests, validates them, persists them,
  and exposes query APIs.
- `scheduler/` computes due work and dispatches executions.
- `worker/` consumes execution messages and runs task handlers.
- `shared-core/` contains domain models, shared contracts, config, utilities, and
  infrastructure adapters that are safe to reuse across services.
- `db-migrations/` owns schema versioning and database bootstrap artifacts.
- `ops/` contains local dev, deployment, and observability assets.
- `docs/` stores architecture decisions and design documents.

## First deliverable

The first design artifact is
`docs/architecture/01-system-boundaries.md`, which defines the service
boundaries and allowed dependencies between them.

The second design artifact is
`docs/architecture/02-concrete-defaults.md`, which locks the initial toolchain,
runtime dependencies, and environment conventions.

The third design artifact is
`docs/architecture/03-domain-model.md`, which defines the core entities,
lifecycles, and state transitions used across the system.

The fourth design artifact is
`docs/architecture/04-interface-contracts.md`, which defines the initial REST,
queue, schema, config, and error contracts.

The fifth design artifact is
`docs/architecture/05-developer-baseline.md`, which defines the formatting,
linting, testing, logging, configuration, and CI baseline for contributors.
