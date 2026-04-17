# worker

Workers consume execution messages and run task handlers asynchronously.

They own execution runtime concerns such as claiming, heartbeats, timeouts,
retries, and graceful shutdown.

## Phase 3 status

Implemented local-mode worker runtime with:

- thread pool execution
- task contract + registry
- execution claim manager (`DISPATCHED -> RUNNING`)
- heartbeat manager
- timeout detection
- result mapping to execution states
- graceful shutdown with drain behavior
- local stress harness (`chronos-worker-stress`)

Current runtime accepts local submissions (in-process) and can execute end-to-end
against in-memory repositories.
