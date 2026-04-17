# shared-core

Shared core contains the reusable domain and infrastructure contracts needed by
multiple Chronos services.

This module must stay free of service-specific orchestration logic.

## Implemented in Phases 1-5

- Domain models and enums (`include/chronos/domain`)
- Enum serialization codecs (`include/chronos/serialization`)
- Time utilities (`include/chronos/time`)
- Retry/backoff helpers + retryable error classification (`include/chronos/retry`)
- Execution state machine (`include/chronos/state_machine`)
- Persistence interfaces + in-memory/postgres scaffolds (`include/chronos/persistence`)
- Messaging contracts + in-memory broker (`include/chronos/messaging`)
- Recovery scanner (`include/chronos/recovery`)
- Idempotency lock store (`include/chronos/idempotency`)

## Build

`shared-core` builds static target:

- `chronos-shared-core`
