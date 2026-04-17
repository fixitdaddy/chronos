# Chaos Test Suite

Run:

```bash
./ops/chaos/run-chaos-suite.sh
```

Scenarios covered:

- kill worker containers
- kill scheduler leader candidate
- pause/unpause RabbitMQ
- pause/unpause Redis
- restart PostgreSQL to simulate failover/reconnect path

Success criteria:

- no silent job loss
- scheduler leadership recovers
- workers resume processing
- retries/dead letters remain observable in metrics/logs
