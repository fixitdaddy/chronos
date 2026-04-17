# Chronos SLO Specification

## SLO 1: Schedule latency

- Objective: 99% of scheduled executions dispatched within 5s of intended schedule time.
- Metric: `chronos_schedule_lag_ms`
- Alert threshold: p99 > 5000ms for 10m.

## SLO 2: Execution success rate

- Objective: >= 99.5% successful executions over rolling 1h.
- Metrics: `chronos_success_total`, `chronos_failure_total`
- Alert threshold: success ratio < 99% over 15m.

## SLO 3: Recovery time

- Objective: scheduler/worker recovery from single-component failure within 2m.
- Metrics: leader absence duration, queue backlog recovery slope.
- Alert threshold: `chronos_scheduler_active_leader < 1` for >2m.

## SLO 4: Alert response

- Objective: operator ACK within 10 minutes for critical alerts.
- Tracked via incident tooling / on-call workflow.

## Error budget policy

- If weekly error budget burn > 50%, freeze risky deployments and prioritize reliability fixes.
