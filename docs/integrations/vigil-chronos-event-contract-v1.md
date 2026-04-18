# chronos.execution.state.changed.v1 event contract (Phase 1.4)

Date: 2026-04-18
Status: contract frozen (implementation scaffold aligned with outbox model)

## Event envelope

- `eventType`: `chronos.execution.state.changed.v1`
- `eventId`: unique immutable event id
- `occurredAt`: ISO-8601 UTC timestamp
- `tenantId`: tenant scope
- `requestId`: request trace id
- `correlationId`: cross-service trace id

## Event payload schema (JSON)

```json
{
  "eventId": "evt_01...",
  "eventType": "chronos.execution.state.changed.v1",
  "occurredAt": "2026-04-18T14:40:00Z",
  "tenantId": "t_acme",
  "requestId": "req_01...",
  "correlationId": "corr_01...",
  "vigilIncidentId": "inc_01...",
  "vigilActionId": "act_01...",
  "chronosJobId": "job_01...",
  "executionId": "exec_01...",
  "fromState": "RUNNING",
  "toState": "SUCCEEDED",
  "attempt": 1,
  "maxAttempts": 5,
  "workerId": "worker-a-3",
  "result": {
    "code": "OK",
    "message": "execution completed"
  }
}
```

## Outbox backing requirements

Each state transition MUST enqueue one outbox row with:
- `event_id` (stable unique)
- `aggregate_type = job_execution`
- `aggregate_id = <executionId>`
- `event_type = chronos.execution.state.changed.v1`
- `payload_json` containing full event payload
- `created_at`, `published_at`, `publish_attempts`

## Dedupe keys

Producer dedupe key:
- `tenantId + ":" + executionId + ":" + toState + ":" + attempt`

Consumer dedupe key (vigil projector):
- `tenantId + ":" + eventId`

## Replay rules

1. Publisher retries from outbox until acked by broker (increment `publish_attempts`).
2. Replayed events must preserve same `eventId` and payload.
3. Consumers must be idempotent:
   - if dedupe key already seen, ack and ignore side effects.
4. If projector misses stream window, reconciliation job polls chronos read endpoints by `vigilActionId`.

## Ordering rules

- Ordering is best-effort per execution.
- Projector must guard against out-of-order arrivals by comparing logical transition precedence.
- Terminal states (`SUCCEEDED`, `FAILED`, `DEAD_LETTER`) are authoritative unless an explicit later compensating event contract is introduced.
