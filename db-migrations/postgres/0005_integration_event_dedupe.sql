-- 0005_integration_event_dedupe.sql
-- Phase 3.3: consumer-side event dedupe table partitioned by tenant + event id scope.

CREATE TABLE IF NOT EXISTS integration_event_dedupe (
  id BIGSERIAL PRIMARY KEY,
  tenant_id TEXT NOT NULL,
  event_id TEXT NOT NULL,
  seen_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT uq_integration_event_dedupe_scope UNIQUE (tenant_id, event_id)
);

CREATE INDEX IF NOT EXISTS idx_integration_event_dedupe_scope
  ON integration_event_dedupe (tenant_id, event_id);

CREATE INDEX IF NOT EXISTS idx_integration_event_dedupe_seen_at
  ON integration_event_dedupe (seen_at DESC);
