# Jenkins CI Migration for chronos

This document records migration from GitHub Actions dependency to Jenkins-based CI.

## Why

GitHub Actions execution can be blocked by billing/account constraints.
Jenkins self-hosted CI removes that dependency and keeps pipeline control local.

## Added assets

- root `Jenkinsfile`
- `scripts/ci/run-ci.sh` (local equivalent of pipeline)
- Jenkins bootstrap compose file:
  - `ops/ci/jenkins/setup-jenkins-docker-compose.yml`
- setup guide:
  - `ops/ci/jenkins/README.md`

## Pipeline stages

1. Checkout
2. Build and test (containerized ubuntu toolchain)
3. Syntax guard (`g++ -fsyntax-only`)
4. Compose config validation (`docker compose config`)

## Operational benefits

- deterministic containerized toolchain
- independent from hosted CI billing lockouts
- easy webhook-based auto-trigger from GitHub
