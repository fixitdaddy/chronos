#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MIGRATIONS_DIR="$ROOT_DIR/postgres"

: "${POSTGRES_HOST:=localhost}"
: "${POSTGRES_PORT:=5432}"
: "${POSTGRES_DB:=chronos}"
: "${POSTGRES_USER:=chronos}"
: "${POSTGRES_PASSWORD:=chronos}"

export PGPASSWORD="$POSTGRES_PASSWORD"

for file in "$MIGRATIONS_DIR"/*.sql; do
  echo "Applying migration: $(basename "$file")"
  psql \
    -h "$POSTGRES_HOST" \
    -p "$POSTGRES_PORT" \
    -U "$POSTGRES_USER" \
    -d "$POSTGRES_DB" \
    -v ON_ERROR_STOP=1 \
    -f "$file"
done

echo "All migrations applied."
