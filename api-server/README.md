# api-server

The API server is the control-plane entrypoint.

It owns request validation, authentication, persistence-facing command handling,
and query endpoints for job and execution state.

## Phase 7 status

Added coordination-backed protections:

- API rate limiting (global bucket)
- per-job execution query caps
- hot-path cache for `GET /jobs/:id`

These are optimization/protection aids and fail-open appropriately when Redis/coordination backend is unavailable.

Dead-letter and quarantine endpoints remain available in local in-memory mode:

- `GET /dead-letter/executions`
- `POST /dead-letter/executions/:execution_id/quarantine`
