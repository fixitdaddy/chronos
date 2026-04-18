# Phase 0 Basic Testing Report (Baseline/Freeze Validation)

Date: 2026-04-18
Scope:
- config validation + startup smoke (both repos)
- API health checks
- baseline load smoke (small synthetic batch)
- observability/metrics signal check

---

## 1) Config validation

Result: **PASS**

- `chronos`: `docker compose config` successful
- `vigil`: `docker compose config` successful

Validated artifacts:
- `/tmp/chronos.compose.validated.yml`
- `/tmp/vigil.compose.validated.yml`

---

## 2) Startup smoke

Result: **PASS (with noted caveat)**

chronos services observed running:
- api-server, postgres, rabbitmq, redis, etcd, prometheus, grafana, elasticsearch, kibana

vigil services observed running:
- api, postgres, redis, prometheus, agent, gitopsd, remediator

Caveat:
- `chronos-scheduler` container exits quickly after tick/log emission instead of staying long-lived. This is acceptable for current MVP runtime pattern but prevents live HTTP metrics scrape on `:9091`.

---

## 3) API health checks

Result: **PASS**

chronos:
- `GET /health` => HTTP 200, body `{"status":"ok","service":"api-server"}`
- `GET /metrics` requires auth token (expected)

vigil:
- `GET /health` => HTTP 200
- `GET /api/v1/ingest/health` => HTTP 200
- `GET /api/v1/actions/health` => HTTP 200

---

## 4) Baseline load smoke (small synthetic batch)

Result: **PASS**

Executed:
- chronos: submitted 50 synthetic jobs via `scripts/scale-tests/load-generator.sh`
- vigil: submitted 10 synthetic actions (`queue_immediately=true`)

Observed counters:
- chronos `chronos_api_jobs_created_total 50`
- vigil action enqueue metrics incremented; queue depth increased

---

## 5) Observability / core metrics check

Result: **PASS (with one runtime caveat)**

chronos API metrics (auth-protected) present:
- `chronos_api_requests_total`
- `chronos_api_jobs_created_total`
- `chronos_schedule_lag_ms`
- `chronos_queue_depth`
- `chronos_success_total`
- `chronos_failure_total`
- `chronos_retry_total`

chronos scheduler metrics:
- available in scheduler logs (not scrape endpoint due to short-lived scheduler process)
- examples: `chronos_scheduler_active_leader`, `chronos_scheduler_dispatch_*`, `chronos_scheduler_scan_lag_ms`

vigil metrics present:
- `requests_total`
- `actions_total`
- `ingest_total`
- `queue_operations_total`
- `queue_length`
- `worker_active`

Prometheus health:
- chronos prometheus `http://localhost:9090/-/healthy` => 200
- vigil prometheus `http://localhost:9092/-/healthy` => 200

Caveat for follow-up:
- scheduler should run as long-lived listener/exporter if we want continuous scrape on `:9091` for production-style monitoring.

---

## Exit Criteria Evaluation

Required:
1) baseline document ✅
- `docs/integrations/vigil-chronos-phase0-1-inventory.md`
- `docs/integrations/vigil-chronos-phase0-2-boundary-freeze.md`
- `docs/integrations/vigil-chronos-phase0-3-non-functional-baseline.md`

2) approved boundary ✅
- captured/frozen in Phase 0.2 boundary document

3) measurable baseline metrics ✅
- core counters/gauges observed from both services + prometheus health checks

Overall Phase-0 basic testing verdict: **PASS (ready to proceed to Phase 1)**

---

## Immediate recommended follow-up tasks (non-blocking)

1. Make scheduler container long-lived with stable metrics endpoint exposure (`:9091`) for continuous scraping.
2. Reduce noisy 4xx baseline in vigil metrics (`/api/v1/ingest/agent/metrics` 422, `/api/v1/actions/gitopsd/events` 404) before integration SLO tracking starts.
3. Add integration-specific metric stubs in both services for Phase 1 (`vigil_chronos_dispatch_latency_ms`, `vigil_chronos_state_lag_seconds`, idempotency counters).
