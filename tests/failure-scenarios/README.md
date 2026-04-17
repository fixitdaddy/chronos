# Failure Scenarios

Local failure scenario harness for Chronos resiliency checks.

Current suite (`failure_scenarios.cpp`) validates:

- duplicate delivery tolerance
- worker kill / stale heartbeat recovery path
- queue reconnect simulation
- DB reconnect simulation

Executable target: `chronos-failure-scenarios` (defined in `worker/CMakeLists.txt`).
