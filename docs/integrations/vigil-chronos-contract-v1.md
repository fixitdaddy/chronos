# vigil -> chronos integration contract v1 (Phase 1.1 + 1.2)

Status: implemented (route + handler scaffold + mandatory headers + schema/error model baseline)
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

## Mandatory headers (Phase 1.2)

All integration endpoints enforce:
- `X-Tenant-Id`
- `Idempotency-Key`
- `X-Request-Id`
- `X-Correlation-Id`
- `X-Vigil-Incident-Id`
- `X-Vigil-Action-Id`

Auth remains inherited from API middleware:
- Bearer token required for non-health routes

## JSON schema + error model (Phase 1.2)

- OpenAPI + schema catalog: `vigil-chronos-contract-v1-openapi.yaml`
- Request schema baseline enforced for create payload keys:
  - `tenantId`, `incident`, `action`, `target`, `execution`, `schedule`, `audit`
- Normalized error shape:
  - `error.code`, `error.message`, `error.httpStatus`, `error.retryable`
  - top-level `requestId`, `correlationId`, `timestamp`

## State mapping + event contract references

- State mapping spec: `vigil-chronos-state-mapping-v1.md`
- Event contract spec: `vigil-chronos-event-contract-v1.md`

## Notes

- Current API implementation is production-minded scaffold; deep semantic DTO parsing, canonical payload hashing, and persistent idempotency store are next implementation steps.
- Contract artifacts are now frozen for Phase 1.2/1.3/1.4 and ready for projector/client implementation in vigil.
