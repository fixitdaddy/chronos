#!/usr/bin/env bash
set -euo pipefail

: "${POSTGRES_HOST:=localhost}"
: "${POSTGRES_PORT:=5432}"
: "${POSTGRES_DB:=chronos}"
: "${POSTGRES_USER:=chronos}"
: "${POSTGRES_PASSWORD:=chronos}"

export PGPASSWORD="$POSTGRES_PASSWORD"

psql -h "$POSTGRES_HOST" -p "$POSTGRES_PORT" -U "$POSTGRES_USER" -d "$POSTGRES_DB" -v ON_ERROR_STOP=1 <<'SQL'
DROP SCHEMA public CASCADE;
CREATE SCHEMA public;
GRANT ALL ON SCHEMA public TO public;
SQL

echo "Database reset complete."
