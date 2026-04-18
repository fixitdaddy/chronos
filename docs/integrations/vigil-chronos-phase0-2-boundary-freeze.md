# vigil ↔ chronos Phase 0.2 Integration Boundary Freeze

Date: 2026-04-18
Status: **FROZEN (v1)**
Applies to: `vigil`, `chronos`

This document locks:
1. service boundaries (what stays internal vs shared)
2. shared integration contract ownership and rules
3. naming conventions (`chronos` lowercase in docs/UI)

---

## 1) Boundary decisions (frozen)

## 1.1 What stays internal to vigil

vigil remains the **decision/control plane**:
- metric ingestion (`/api/v1/ingest*`)
- policy evaluation and violation detection
- incident lifecycle and action intent lifecycle
- policy-driven escalation, suppression, manual approvals
- action projector/reconciler that reflects chronos execution state into vigil action/incident state
- vigil-specific UI/API for incident/action query and operator workflows

vigil internal statuses remain:
- `pending`, `running`, `completed`, `failed`, `cancelled` (+ optional integration-specific projected substatus)

## 1.2 What stays internal to chronos

chronos remains the **scheduling/execution plane**:
- durable jobs/schedules/executions persistence (PostgreSQL source of truth)
- scheduler leadership/fencing with etcd
- transient queue delivery/execution dispatch via RabbitMQ
- advisory optimization via Redis (fail-open where non-critical)
- retry/backoff/dead-letter/recovery loops
- worker runtime, task execution, timeout handling
- execution telemetry and outbox events

## 1.3 Shared integration surface only (cross-service contract)

Cross-service calls are restricted to:
- **vigil → chronos** integration APIs
- **chronos → vigil** execution-state events (or polling fallback)

No direct vigil writes to chronos internal tables.
No direct chronos writes to vigil internal tables.
No shared DB.

---

## 2) Shared contract ownership

## 2.1 API ownership

- Endpoint host: **chronos**
- Contract owner: **joint**, canonical files stored under:
  - `chronos/docs/integrations/`
- vigil consumes this contract via generated/manual client module.

## 2.2 Event ownership

- Event producer: **chronos** (outbox-backed)
- Event consumer/projector: **vigil**
- Delivery transport: RabbitMQ exchange/topic as agreed in contract
- Replay/reconciliation authority: vigil reconciler may pull chronos read APIs for healing

## 2.3 Contract versioning

- Semantic contract version in path/schema ID: `v1`
- Additive changes allowed in `v1` (new optional fields)
- Breaking changes require `v2`

---

## 3) Data/identity boundaries (frozen)

## 3.1 Tenant model

Mandatory tenant propagation on all integration APIs/events:
- `tenantId` in payload
- `X-Tenant-Id` header
- request rejected if mismatch

Tenant isolation rule:
- no cross-tenant read/write visibility in either service

## 3.2 Correlation/tracing fields

Mandatory cross-service fields:
- `X-Request-Id`
- `X-Correlation-Id`
- `traceparent` (W3C)
- optional `baggage`

Must be persisted into chronos execution metadata and emitted in events.

## 3.3 Idempotency (mandatory)

Required header:
- `Idempotency-Key`

Enforcement scope:
- `(tenant_id, endpoint, idempotency_key)`

Behavior:
- same key + same canonical payload => replay prior accepted response
- same key + different payload hash => hard conflict (`409`)

---

## 4) Execution path boundary (migration-safe)

During migration, vigil supports dual path behind feature flags:
- legacy internal queue/remediator path (existing)
- chronos integration path (new)

Frozen rule:
- for any tenant/incident/action, only one active execution path at a time
- cutover controlled by tenant-scoped feature flag

Target end state:
- vigil decisions dispatch to chronos integration API
- chronos executes and emits state
- vigil projects final state to incident/action model

---

## 5) Naming conventions freeze

## 5.1 Product/service naming in docs/UI

- Use `chronos` (lowercase) in prose, UI labels, operator docs
- Use `vigil` (lowercase) in prose, UI labels, operator docs
- Use `vigil → chronos` format for integration direction

Examples:
- ✅ "vigil sends remediation jobs to chronos"
- ❌ "Vigil sends remediation jobs to Chronos"

## 5.2 Code-level exceptions

Language/runtime naming standards remain unchanged:
- class/type names may be PascalCase
- enum/constants may be UPPER_SNAKE_CASE
- package/module paths follow language conventions

Only docs/UI/operator text are forced to lowercase service names.

## 5.3 Canonical identifiers

Adopt these logical IDs in integration docs/schemas:
- `tenantId`
- `vigilIncidentId`
- `vigilActionId`
- `chronosJobId`
- `executionId`
- `remediationJobId`

---

## 6) Explicit non-goals (for this freeze)

Not changing in Phase 0.2:
- no replacement of internal vigil policy model
- no chronos core state machine redesign
- no shared database creation
- no RabbitMQ/Redis/etcd role reversal

---

## 7) Phase 0.2 output checklist

- [x] service boundary responsibilities frozen
- [x] shared contract ownership frozen
- [x] tenant/correlation/idempotency boundary frozen
- [x] migration dual-path rule frozen
- [x] naming conventions (`chronos` lowercase in docs/UI) frozen

Next: Phase 0.3 non-functional baseline + SLO gates.
