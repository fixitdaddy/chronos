# vigil -> chronos integration contract v1 (Phase 1.1)

Status: implemented (route + handler scaffold)
Date: 2026-04-18

## Endpoints

1. `POST /v1/integrations/vigil/remediation-jobs`
2. `GET /v1/integrations/vigil/remediation-jobs/{id}`
3. `POST /v1/integrations/vigil/remediation-jobs/{id}:cancel`
4. `GET /v1/integrations/vigil/actions/{vigilActionId}`

## Current implementation status

Implemented in API server routing + handlers:
- route registration in `api-server/src/http/server.cpp`
- handler scaffold in `api-server/src/handlers/vigil_integration_handlers.cpp`
- public handler header in `api-server/include/chronos/api/handlers/vigil_integration_handlers.hpp`

## Header requirements currently enforced for create endpoint

`POST /v1/integrations/vigil/remediation-jobs` requires:
- `X-Tenant-Id`
- `Idempotency-Key`
- `X-Vigil-Incident-Id`
- `X-Vigil-Action-Id`

Auth remains inherited from API middleware:
- Bearer token required for non-health routes

## Notes

- This Phase 1.1 implementation provides contract route availability and deterministic response shapes.
- Full persistence-backed idempotency, strict payload canonicalization/hash conflict detection, and stateful lookups are scheduled for next contract subphases.
- Canonical schema is versioned in `vigil-chronos-contract-v1-openapi.yaml`.
