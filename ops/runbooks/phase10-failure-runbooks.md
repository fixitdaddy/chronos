# Phase 10 Runbooks

## 1) Dead letters

1. Query dead-letter inspection endpoint (`/dead-letter/executions`) or broker DLQ dashboard.
2. Group by error code and task type.
3. Decide action:
   - replay after fix
   - quarantine poison jobs
   - mark permanently failed
4. Track trend in retries/dead-letter metrics.

## 2) Stuck executions

1. Inspect `RUNNING` with stale heartbeat and `DISPATCHED` age.
2. Run recovery scanner / reconciliation.
3. Verify transitions to `RETRY_PENDING` and republish events.
4. Confirm resumed processing and no duplicate corruption.

## 3) Queue congestion

1. Check queue depth growth and dispatch/consume rates.
2. Scale workers horizontally.
3. Verify DB latency and task latency; identify bottleneck.
4. Apply temporary per-tenant throttles if needed.

## 4) Failover issues

1. Validate scheduler leader election status.
2. Check lease churn and active leader metric.
3. If no leader, restart one scheduler instance first.
4. Confirm fencing prevents old-leader dispatch.
5. Validate no missed/stranded jobs via execution history.
