#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required"
  exit 1
fi

echo "[ci] containerized build + test"
docker run --rm \
  -v "$PWD:/src" \
  -w /src \
  ubuntu:24.04 \
  bash -lc '
    set -euxo pipefail
    apt-get update
    apt-get install -y --no-install-recommends build-essential cmake ninja-build g++
    cmake -S . -B build -G Ninja -DCHRONOS_BUILD_TESTS=ON
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
  '

echo "[ci] syntax guard"
docker run --rm \
  -v "$PWD:/src" \
  -w /src \
  ubuntu:24.04 \
  bash -lc '
    set -euxo pipefail
    apt-get update
    apt-get install -y --no-install-recommends g++ findutils
    for f in $(find shared-core api-server scheduler worker tests -type f -name "*.cpp" | sort); do
      g++ -std=c++20 -fsyntax-only -Ishared-core/include -Iapi-server/include -Ischeduler/include -Iworker/include "$f"
    done
  '

echo "[ci] compose validation"
docker compose -f docker-compose.yml config >/tmp/chronos-compose.validated.yml

echo "[ci] all checks passed ✅"
