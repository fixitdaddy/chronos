# vigil ↔ chronos Phase 0.3 Non-Functional Baseline

Date: 2026-04-18
Status: **BASELINE FROZEN (v1)**
Scope: integration path from vigil decisioning to chronos execution

This baseline defines:
1. target SLOs
2. expected throughput assumptions
3. failure budget assumptions + burn policy
4. rollout flag strategy and rollback points

---

## 1) Service-level objectives (SLO targets)

These SLOs apply to the integrated path (vigil trigger → chronos execution state projected back to vigil).

## SLO-1 Dispatch latency (chronos scheduling path)

Objective:
- 99% of remediation-triggered executions reach `DISPATCHED` within **5 seconds** from accepted integration request time.

Metric signals:
- `chronos_schedule_lag_ms` (existing)
- integration metric to add: `vigil_chronos_dispatch_latency_ms`

Alert trigger (baseline):
- p99 dispatch latency > 5000ms for 10m.

## SLO-2 Execution completion success ratio

Objective:
- >= **99.0%** execution completion without terminal failure across rolling 1h for normal operating windows.

Metric signals:
- `chronos_success_total`, `chronos_failure_total` (existing)
- integration split by source=`vigil` tag (to add)

Alert trigger:
- success ratio < 99.0% for 15m.

## SLO-3 End-to-end state propagation lag

Objective:
- 99% of chronos execution state transitions are reflected in vigil action projector within **10 seconds**.

Metric signals (to add):
- `vigil_chronos_state_lag_seconds`
- `vigil_projector_queue_lag_seconds`

Alert trigger:
- p99 state lag > 10s for 10m.

## SLO-4 Control-plane availability for integration API

Objective:
- chronos integration endpoint availability >= **99.9% monthly**.

Measurement:
- request success for non-4xx integration API calls.

Alert trigger:
- 5xx ratio > 1% for 10m.

## SLO-5 Idempotency correctness

Objective:
- duplicate submission should not create duplicate execution effects (business-level exactly-once by idempotency semantics) for 100% of replayed keys.

Measurement:
- `vigil_chronos_idempotency_replays_total`
- `vigil_chronos_idempotency_conflicts_total`
- sampled duplicate-storm test assertions

Alert trigger:
- any detected duplicate effect for same `(tenant, idempotency_key, endpoint)`.

## SLO-6 Tenant isolation safety

Objective:
- 0 cross-tenant data exposure incidents.

Measurement:
- negative test pass rate
- production security event counters

Alert trigger:
- any confirmed cross-tenant read/write leakage = sev0 rollback condition.

---

## 2) Throughput and load assumptions (baseline)

Initial production-minded planning envelope (tune after load tests):

Steady state:
- 20 remediation requests/sec (vigil→chronos)
- 200 execution state events/sec sustained

Burst state (incident storm):
- 100 remediation requests/sec for up to 10 minutes
- queue depth may spike, but must recover to baseline within 15 minutes after burst ends

Per-tenant fairness assumption:
- no single tenant should consume >40% scheduler dispatch capacity without explicit override

Latency under burst:
- temporary p95 degradation acceptable up to 2x steady-state p95
- p99 must recover to SLO threshold within 15 minutes

---

## 3) Failure budget assumptions and burn policy

Budget windows:
- monthly primary budget for availability/error-rate SLOs
- weekly burn tracking for operational action

Working assumptions:
- SLO-4 (99.9% availability) allows ~43m unavailability/month
- SLO-2 (99.0 success ratio) allows up to 1.0% failure envelope before policy breach

Burn-rate policy:

Green:
- <25% weekly budget burn
- normal feature rollout allowed

Yellow:
- 25–50% weekly burn
- reduce rollout speed, mandatory reliability review

Orange:
- 50–75% weekly burn
- freeze risky changes, only reliability/bugfix releases

Red:
- >75% weekly burn or sev0 isolation/idempotency breach
- immediate rollout halt, incident mode, rollback to last stable configuration

Hard-stop conditions (override):
- any cross-tenant leakage event
- duplicate side effects violating idempotency guarantee
- sustained no-leader condition >5m in production

---

## 4) Rollout flag strategy

## 4.1 Flags to introduce

In vigil:
- `chronosIntegrationV1Enabled` (master switch)
- `chronosIntegrationV1TenantAllowlist` (tenant-scoped rollout)
- `chronosIntegrationV1Mode` = `shadow|active`
  - `shadow`: call chronos and compare/probe, but do not switch authoritative action finalization
  - `active`: chronos is authoritative execution path

In chronos:
- `integration.vigil.v1.enabled`
- `integration.vigil.v1.enforceStrictHeaders`
- `integration.vigil.v1.enforceIdempotencyConflictHardFail`

## 4.2 Rollout sequence (frozen)

Step 1: dark deploy
- deploy integration code with flags off

Step 2: shadow mode (single internal tenant)
- validate contract, headers, tracing, projector lag

Step 3: active canary (1 tenant)
- low-volume tenant only

Step 4: active cohort expansion
- 5% tenant cohort → 25% → 50% → 100%

Step 5: legacy path deprecation
- keep fallback window for 1 week after 100%

---

## 5) Rollback points and decision gates

## RP-1 (after shadow mode)
Rollback if:
- contract/schema mismatch rate > 0.5%
- p99 projector lag > 20s for 15m

Action:
- disable vigil active mode; keep only legacy path

## RP-2 (after canary active tenant)
Rollback if any:
- duplicate effects detected
- success ratio for canary < 98.5% for 15m
- no-leader incidents repeated >2 within 30m

Action:
- tenant-level rollback via allowlist removal

## RP-3 (during 25–50% cohort)
Rollback if:
- 5xx > 1.5% for 10m
- queue recovery exceeds 20m post burst
- dead-letter rate doubles baseline for >30m

Action:
- freeze expansion, revert cohort to last stable percentage

## RP-4 (post-100% cutover stabilization window)
Rollback if:
- any sev0 isolation/security event
- burn-rate enters red zone and remains >2h despite mitigation

Action:
- switch `chronosIntegrationV1Enabled=false`, reactivate legacy path per tenant emergency profile

---

## 6) Baseline test gates (for Phase 0 closure)

Must pass before moving to Phase 1 implementation:
- SLO definitions published and reviewed
- load model approved (steady + burst)
- alert thresholds mapped to existing/new metrics
- feature flags defined in both repos config surfaces
- rollback runbook with explicit RP-1..RP-4 actions documented

---

## 7) Immediate implementation implications

From this baseline, next-phase implementation must include:
- integration metrics additions in both services
- tenant-scoped feature flag plumbing in vigil
- strict header/idempotency toggles in chronos integration routes
- projector lag measurement and dashboard panel
- rollback automation hooks (tenant allowlist downgrade path)

This completes Phase 0.3 non-functional freeze input.
