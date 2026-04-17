# api-server

The API server is the control-plane entrypoint.

It owns request validation, authentication, persistence-facing command handling,
and query endpoints for job and execution state.

## Phase 5 status

In addition to core job/schedule/execution APIs, Phase 5 adds dead-letter and quarantine endpoints (local in-memory mode):

- `GET /dead-letter/executions`
- `POST /dead-letter/executions/:execution_id/quarantine`

These support failure triage and poison-job containment flows.
