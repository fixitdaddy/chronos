BEGIN;

INSERT INTO jobs (
  job_id,
  name,
  task_type,
  payload,
  priority,
  queue_name,
  max_execution_time_seconds,
  retry_policy,
  idempotency_key,
  job_state
)
VALUES (
  '11111111-1111-1111-1111-111111111111',
  'dev-sample-job',
  'sample.echo',
  '{"message":"hello chronos"}'::jsonb,
  1,
  'main_queue',
  120,
  '{"max_attempts":3,"backoff_strategy":"EXPONENTIAL","initial_delay_seconds":5,"max_delay_seconds":60,"backoff_multiplier":2.0}'::jsonb,
  'dev-sample-idempotency-key',
  'ACTIVE'
)
ON CONFLICT (job_id) DO NOTHING;

INSERT INTO job_schedules (
  schedule_id,
  job_id,
  schedule_type,
  run_at,
  timezone,
  next_run_at,
  misfire_policy,
  active
)
VALUES (
  '22222222-2222-2222-2222-222222222222',
  '11111111-1111-1111-1111-111111111111',
  'ONE_TIME',
  NOW() + INTERVAL '5 minutes',
  'UTC',
  NOW() + INTERVAL '5 minutes',
  'FIRE_ONCE',
  TRUE
)
ON CONFLICT (schedule_id) DO NOTHING;

COMMIT;
