#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SEED_FILE="$ROOT_DIR/postgres/0003_seed_dev.sql"

: "${POSTGRES_HOST:=localhost}"
: "${POSTGRES_PORT:=5432}"
: "${POSTGRES_DB:=chronos}"
: "${POSTGRES_USER:=chronos}"
: "${POSTGRES_PASSWORD:=chronos}"

export PGPASSWORD="$POSTGRES_PASSWORD"

psql \
  -h "$POSTGRES_HOST" \
  -p "$POSTGRES_PORT" \
  -U "$POSTGRES_USER" \
  -d "$POSTGRES_DB" \
  -v ON_ERROR_STOP=1 \
  -f "$SEED_FILE"

echo "Seed applied."
