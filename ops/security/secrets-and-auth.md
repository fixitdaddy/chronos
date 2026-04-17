# Production Security Hardening

## Secret handling

- Use Kubernetes Secrets / external secret manager (Vault, AWS Secrets Manager, GCP Secret Manager).
- Never commit plaintext credentials in repo.
- Rotate credentials for PostgreSQL, RabbitMQ, Redis, etcd, JWT signing keys.

## TLS

- Terminate TLS at ingress (or service mesh) and enforce HTTPS for API.
- Enable TLS for PostgreSQL/RabbitMQ/Redis/etcd in production clusters.
- Pin internal CA trust chain where possible.

## Auth hardening

- Replace static bearer token with JWT/OIDC validation.
- Enforce token expiration + audience + issuer checks.
- Enable tenant-scoped authorization checks in API handlers.

## Audit logging

- Audit all control-plane mutations: job create/update/pause/resume/delete, schedule changes, execution cancellation/quarantine.
- Include actor id, tenant id, request id, trace id, and before/after state summary.
