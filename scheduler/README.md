# scheduler

The scheduler is the orchestration component that detects due work and dispatches
execution attempts.

It is the only component that decides when a job should run.

## Phase 7 status

Added Redis-coordination-style protections in dispatch flow:

- optional distributed execution-window lock use
- dispatch-window dedupe keys
- fail-open behavior for non-critical coordination paths

Correctness still depends on durable DB state transitions and queue workflows; coordination is additive, not authoritative.

Distributed scheduler (leader/follower/fencing/runtime metrics) remains active from Phase 6.
