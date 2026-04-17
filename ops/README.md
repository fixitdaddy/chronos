# ops

Operations assets live here, including local development setup, container build
definitions, deployment manifests, observability bootstrap, and runbooks.

## Observability stack (Phase 8)

- Prometheus scrape + alert rules:
  - `ops/observability/prometheus/prometheus.yml`
  - `ops/observability/alerts/chronos-alerts.yml`
- Grafana dashboard JSON:
  - `ops/observability/grafana/dashboards/chronos-overview.json`
- Elastic ingestion via Filebeat:
  - `ops/observability/elastic/filebeat/filebeat.yml`
- Local compose for observability stack:
  - `ops/observability/compose-observability.yml`
- Runbooks:
  - `ops/runbooks/observability-operability.md`

## Production hardening (Phase 10)

- security policy and secret/TLS/auth guidance:
  - `ops/security/secrets-and-auth.md`
- chaos suite:
  - `ops/chaos/run-chaos-suite.sh`
  - `ops/chaos/README.md`
- failure runbooks:
  - `ops/runbooks/phase10-failure-runbooks.md`
