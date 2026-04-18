# vigil ↔ chronos state mapping specification v1 (Phase 1.3)

Date: 2026-04-18
Status: frozen for contract v1

This table defines mapping from chronos execution states to vigil action states and incident impact semantics.

## Mapping table

| chronos execution state | vigil action state | incident impact | escalation flag |
|---|---|---|---|
| `PENDING` | `queued` | `open` | `false` |
| `DISPATCHED` | `dispatched` | `open` | `false` |
| `RUNNING` | `in_progress` | `open` | `false` |
| `RETRY_PENDING` | `retrying` | `open` | `false` |
| `SUCCEEDED` | `succeeded` | `resolved_candidate` | `false` |
| `FAILED` | `failed` | `escalated` | `true` |
| `DEAD_LETTER` | `dead_lettered` | `escalated` | `true` |

## Rules

1. `resolved_candidate` means vigil policy layer decides whether incident can be moved to final `resolved`.
2. `FAILED` and `DEAD_LETTER` always set escalation flag true.
3. `RETRY_PENDING` keeps incident open and must not escalate by default.
4. If chronos introduces new execution states, mapping must be explicitly extended before enabling new state in projector.

## Canonical payload fields for projector

Required from chronos state-event/read model:
- `tenantId`
- `vigilIncidentId`
- `vigilActionId`
- `chronosJobId`
- `executionId`
- `executionState`
- `attempt`
- `maxAttempts`
- `occurredAt`
- `correlationId`
- `requestId`
