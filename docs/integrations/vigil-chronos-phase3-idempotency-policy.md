# Phase 3 Idempotency Policy (vigil -> chronos)

Date: 2026-04-18
Status: implemented (in-memory runtime) + DB migration scaffold

## Scope key

Idempotency scope is:
- `(tenant_id, endpoint, idempotency_key)`

Endpoint currently implemented:
- `POST:/v1/integrations/vigil/remediation-jobs`

## Canonical payload hash

Stored as:
- `canonical_payload_hash`

Current canonicalization:
- stable hash over request body string (`ComputeCanonicalPayloadHash`)

Planned hardening:
- switch to JSON canonical form normalization before hashing.

## Behavior

1. First request for scope key:
- process request
- persist response body + canonical hash
- return `202` + `Idempotency-Replayed: false`

2. Replay with same scope key and same canonical hash:
- return persisted response body
- return `202` + `Idempotency-Replayed: true`

3. Replay with same scope key and different canonical hash:
- return `409 CHRONOS_IDEMPOTENCY_MISMATCH`

## Storage backends

- In-memory repository (active runtime in current api-server main)
- Postgres migration table scaffold:
  - `integration_idempotency_keys`

## Migration artifact

- `db-migrations/postgres/0004_integration_idempotency_keys.sql`
