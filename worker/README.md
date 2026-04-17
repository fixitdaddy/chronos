# worker

Workers consume execution messages and run task handlers asynchronously.

They own execution runtime concerns such as claiming, heartbeats, timeouts,
retries, and graceful shutdown.
