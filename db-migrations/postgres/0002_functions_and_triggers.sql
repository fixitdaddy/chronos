BEGIN;

CREATE OR REPLACE FUNCTION chronos_touch_updated_at()
RETURNS TRIGGER AS $$
BEGIN
  NEW.updated_at = NOW();
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS trg_jobs_touch_updated_at ON jobs;
CREATE TRIGGER trg_jobs_touch_updated_at
BEFORE UPDATE ON jobs
FOR EACH ROW
EXECUTE FUNCTION chronos_touch_updated_at();

DROP TRIGGER IF EXISTS trg_job_schedules_touch_updated_at ON job_schedules;
CREATE TRIGGER trg_job_schedules_touch_updated_at
BEFORE UPDATE ON job_schedules
FOR EACH ROW
EXECUTE FUNCTION chronos_touch_updated_at();

DROP TRIGGER IF EXISTS trg_job_executions_touch_updated_at ON job_executions;
CREATE TRIGGER trg_job_executions_touch_updated_at
BEFORE UPDATE ON job_executions
FOR EACH ROW
EXECUTE FUNCTION chronos_touch_updated_at();

DROP TRIGGER IF EXISTS trg_job_attempts_touch_updated_at ON job_attempts;
CREATE TRIGGER trg_job_attempts_touch_updated_at
BEFORE UPDATE ON job_attempts
FOR EACH ROW
EXECUTE FUNCTION chronos_touch_updated_at();

COMMIT;
