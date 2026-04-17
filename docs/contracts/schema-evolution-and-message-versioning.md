# Schema Evolution & Message Versioning Strategy

## DB schema evolution

1. Expand first
   - add nullable/new columns, new indexes, new tables without breaking old code.
2. Dual-write / dual-read phase
   - write old+new representations where needed.
3. Migrate data in background
   - idempotent migration jobs; no long blocking transactions.
4. Contract
   - remove old fields only after all services are upgraded.

## Message versioning

- Keep `message_type` + `message_version` in every queue payload.
- Consumers must:
  - accept current + previous version during rollout,
  - reject unknown major versions to dead-letter queue with explicit error code.
- Producers should emit latest stable version.

## Compatibility rules

- Adding optional fields: backward compatible.
- Removing required fields: breaking (requires major version).
- Changing semantic meaning of existing field: breaking.

## Rollout order

1. Deploy consumers that understand both versions.
2. Deploy producers emitting new version.
3. Monitor dead-letter compatibility errors.
4. Retire old version after drain window.
