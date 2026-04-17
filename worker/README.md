# worker

Workers consume execution messages and run task handlers asynchronously.

They own execution runtime concerns such as claiming, heartbeats, timeouts,
retries, and graceful shutdown.

## Phase 5 status

Added retry/recovery/failure-management behaviors:

- retryable-error classification + backoff-aware dead-letter decisions
- runtime idempotency lock around task execution
- poison-job detector with quarantine threshold
- failure scenario harness (`chronos-failure-scenarios`)

Consumer and runtime continue to ensure durable state transition before queue ack.
