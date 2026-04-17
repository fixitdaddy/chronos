# Redis Coordination Tests

Validates coordination behavior expected for Redis-backed protections:

- distributed lock expiry and recovery
- network-partition style availability loss behavior

Key contract:
- Redis coordination is optimization/aid, not source of truth
- system remains correct during temporary Redis unavailability
