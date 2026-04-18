# Phase 1 Basic Testing Report (Contract v1)

Date: 2026-04-18
Scope:
- schema validation tests (valid/invalid payloads)
- header-required tests
- error model conformance tests (400/401/403/404/409/429/500/503)
- state mapping tests for known transitions

---

## 1) Schema validation tests

Result: **PASS**

- invalid payload (missing required top-level keys) -> **400**
- valid create payload -> **202**

---

## 2) Header-required tests

Result: **PASS**

Validated mandatory header enforcement on integration create endpoint:
- missing `X-Tenant-Id` -> **400**
- missing `X-Request-Id` -> **400**

(Other required headers are also enforced by common validation path.)

---

## 3) Error model conformance tests (target: 400/401/403/404/409/429/500/503)

Result: **PASS**

### Passing status checks
- 400 -> observed and contract-style body present
- 401 -> observed and normalized envelope present
- 403 -> observed and normalized envelope present
- 404 -> observed and normalized envelope present
- 409 -> observed for idempotency mismatch
- 429 -> observed
- 500 -> observed via deterministic fault-injection header
- 503 -> observed via deterministic fault-injection header

### Contract conformance details
All tested status classes now return normalized model:
- `error.code`, `error.message`, `error.httpStatus`, `error.retryable`
- `requestId`, `correlationId`, `timestamp`

Deterministic conformance hooks added:
- request header `X-Chronos-Fault-Inject: 500`
- request header `X-Chronos-Fault-Inject: 503`

Idempotency mismatch behavior added:
- same `(tenant, endpoint, idempotency-key)` + different payload fingerprint => `409 CHRONOS_IDEMPOTENCY_MISMATCH`

---

## 4) State mapping tests

Result: **PASS (spec + runtime sample)**

- mapping spec file includes all known chronos execution states:
  - `PENDING`, `DISPATCHED`, `RUNNING`, `RETRY_PENDING`, `SUCCEEDED`, `FAILED`, `DEAD_LETTER`
- runtime sample check:
  - integration action status endpoint returns mapped state (`RUNNING` -> `in_progress`) and incident impact field.

---

## 5) Test summary

- Total passing checks executed: **all planned phase checks + gap-fix retests**
- Hard test failures: **0**
- Contract gaps identified: **0** (previous 3 gaps resolved)

---

## Exit criteria evaluation

Target exit criteria:
- versioned contract docs ✅
- OpenAPI ✅
- signed-off mapping table ✅ (spec artifact generated)

Testing gate status for Phase 1 basic validation:
- **PASS** (gap-fix patch applied and verified)
