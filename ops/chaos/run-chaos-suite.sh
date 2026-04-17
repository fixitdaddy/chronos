#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

COMPOSE="docker compose"

echo "[chaos] starting stack"
$COMPOSE up -d --build

echo "[chaos] kill worker"
$COMPOSE kill worker || true
sleep 5
$COMPOSE up -d --scale worker=3

echo "[chaos] kill leader candidate (scheduler)"
$COMPOSE kill scheduler || true
sleep 5
$COMPOSE up -d scheduler

echo "[chaos] pause RabbitMQ"
$COMPOSE pause rabbitmq || true
sleep 15
$COMPOSE unpause rabbitmq || true

echo "[chaos] pause Redis"
$COMPOSE pause redis || true
sleep 15
$COMPOSE unpause redis || true

echo "[chaos] simulate DB failover (restart postgres)"
$COMPOSE restart postgres
sleep 20

echo "[chaos] collect quick health snapshot"
$COMPOSE ps
curl -s http://localhost:8080/health || true
echo
curl -s http://localhost:8080/metrics | head -n 80 || true
echo
curl -s http://localhost:9091/metrics | head -n 80 || true

echo "[chaos] completed"
