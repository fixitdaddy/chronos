-- 0004_integration_idempotency_keys.sql
-- Phase 3.1: integration idempotency table for vigil -> chronos endpoints.

CREATE TABLE IF NOT EXISTS integration_idempotency_keys (
  id BIGSERIAL PRIMARY KEY,
  tenant_id TEXT NOT NULL,
  endpoint TEXT NOT NULL,
  idempotency_key TEXT NOT NULL,
  canonical_payload_hash TEXT NOT NULL,
  response_body JSONB NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  CONSTRAINT uq_integration_idempotency_scope UNIQUE (tenant_id, endpoint, idempotency_key)
);

CREATE INDEX IF NOT EXISTS idx_integration_idempotency_scope
  ON integration_idempotency_keys (tenant_id, endpoint, idempotency_key);

CREATE INDEX IF NOT EXISTS idx_integration_idempotency_created_at
  ON integration_idempotency_keys (created_at DESC);

-- keep updated_at fresh on write
CREATE OR REPLACE FUNCTION set_integration_idempotency_updated_at()
RETURNS TRIGGER AS $$
BEGIN
  NEW.updated_at = NOW();
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_integration_idempotency_updated_at
  ON integration_idempotency_keys;

CREATE TRIGGER trg_integration_idempotency_updated_at
BEFORE UPDATE ON integration_idempotency_keys
FOR EACH ROW
EXECUTE FUNCTION set_integration_idempotency_updated_at();
