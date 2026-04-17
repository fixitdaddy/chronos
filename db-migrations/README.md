# db-migrations

This directory owns PostgreSQL schema evolution for Chronos.

Runtime services depend on the schema, but migration files remain isolated from
runtime application logic.

## Layout

- `postgres/0001_init.sql`: baseline schema (jobs, schedules, executions, attempts, heartbeats, audit, outbox)
- `postgres/0002_functions_and_triggers.sql`: updated_at trigger utilities
- `postgres/0003_seed_dev.sql`: deterministic dev seed data
- `scripts/migrate.sh`: applies all SQL migrations in lexical order
- `scripts/reset_dev.sh`: resets the `public` schema (dev only)
- `scripts/seed_dev.sh`: applies dev seed file

## Usage

```bash
# from repo root
export POSTGRES_HOST=localhost
export POSTGRES_PORT=5432
export POSTGRES_DB=chronos
export POSTGRES_USER=chronos
export POSTGRES_PASSWORD=chronos

./db-migrations/scripts/migrate.sh
./db-migrations/scripts/seed_dev.sh
```

For a clean local reset:

```bash
./db-migrations/scripts/reset_dev.sh
./db-migrations/scripts/migrate.sh
./db-migrations/scripts/seed_dev.sh
```
