# Phase 3.3 / 3.4 — event dedupe + retention policy

Date: 2026-04-18
Status: implemented (in-memory runtime) + migration scaffolds for Postgres path

## 3.3 Event dedupe support

### Consumer dedupe key strategy

Scope key:
- `tenant_id + ":" + event_id`

Repository contract:
- `TryMarkSeen(tenant_id, event_id)` -> true if first-seen, false if duplicate
- `IsSeen(tenant_id, event_id)`

Implementations:
- `InMemoryEventDedupeRepository` (active in current api runtime)
- `PostgresEventDedupeRepository` (scaffold with SQL TODOs)

DB migration artifact:
- `db-migrations/postgres/0005_integration_event_dedupe.sql`
- unique constraint on `(tenant_id, event_id)`

## 3.4 TTL / retention policy

### Retention objects

1) Idempotency entries
- repo: `IIntegrationIdempotencyRepository`
- purge API: `PurgeExpired(ttl)`

2) Event dedupe artifacts
- repo: `IEventDedupeRepository`
- purge API: `PurgeExpired(ttl)`

### Default TTL policy (current)

- idempotency retention: **7 days**
- event dedupe retention: **2 days**

### Retention execution endpoint

Operational endpoint added:
- `POST /v1/integrations/vigil/maintenance/retention:run`

Behavior:
- runs retention purge for idempotency + dedupe repositories
- returns count summary:
  - `idempotencyRemoved`
  - `dedupeRemoved`

## Notes

- In current api-server main runtime, repositories are in-memory; retention is effective per-process lifetime.
- Postgres repository paths are scaffolded and can be enabled once main runtime switches from in-memory integration stores.
