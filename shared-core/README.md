# shared-core

Shared core contains the reusable domain and infrastructure contracts needed by
multiple Chronos services.

This module must stay free of service-specific orchestration logic.

## Implemented in Phase 1

- Domain models and enums (`include/chronos/domain`)
- Enum serialization codecs (`include/chronos/serialization`)
- Time utilities (`include/chronos/time`)
- Retry/backoff helpers (`include/chronos/retry`)
- Canonical execution state machine (`include/chronos/state_machine`)
- Persistence repository interfaces (`include/chronos/persistence/repository.hpp`)
- PostgreSQL repository adapter skeleton (`include/chronos/persistence/postgres`)

## Build

`shared-core` now builds a static library target:

- `chronos-shared-core`

and test executables:

- `chronos-state-machine-tests`
- `chronos-backoff-tests`
