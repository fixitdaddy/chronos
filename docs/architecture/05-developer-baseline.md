# Developer Baseline

This document locks the initial developer workflow baseline for Chronos.

The goal of this phase is to give every contributor the same coding, testing,
and validation foundation before feature work begins.

## Chosen defaults

### Formatting

- Primary formatter: `clang-format`
- Style file: repo-root `.clang-format`
- Scope: all `*.h`, `*.hpp`, `*.cc`, `*.cpp`, `*.cxx`

### Linting

- Primary linter: `clang-tidy`
- Config file: repo-root `.clang-tidy`
- Scope: service and shared-core source trees

### Unit testing

- Test framework: `doctest`
- Test runner: `ctest`
- Test layout: module-local tests under each component's `tests/`

### Logging

- Logging library: `spdlog`
- Contract: structured key-value friendly application logs with stable fields

### Configuration loading

- Config file format: `YAML`
- Config parsing library: `yaml-cpp`
- Precedence model: environment variables override YAML file values

### CI

- CI provider baseline: `GitHub Actions`
- First pipeline stages:
  - configure
  - build
  - test
  - lint

## Why these defaults

### `clang-format`

- Standard tool for C++ formatting.
- Low-friction IDE integration.
- Keeps diffs small and consistent.

### `clang-tidy`

- Gives useful static analysis for modern C++ correctness and style drift.
- Works well with CMake-generated compile commands.

### `doctest`

- Lightweight and easy to embed in a new C++ codebase.
- Minimal compile and integration overhead for an early-stage multi-binary repo.

### `spdlog`

- Mature, widely used, and simple to wrap behind shared logging interfaces.
- Supports structured fields and multiple sinks cleanly.

### `yaml-cpp`

- Common fit for human-readable service configuration.
- Keeps local development config readable while still allowing environment
  overrides.

## Repo-level baseline files

- `.editorconfig`: whitespace and newline defaults
- `.clang-format`: canonical C++ formatting rules
- `.clang-tidy`: baseline lint profile
- `CMakePresets.json`: standard configure and build entrypoints
- `.github/workflows/ci.yml`: initial CI skeleton

## Coding conventions

- C++ standard: `C++20`
- Prefer headers in `include/` and implementation files in `src/`
- Avoid service-to-service includes that bypass `shared-core`
- Use UTC timestamps for all persisted and logged time values
- Prefer explicit enums and named structs over anonymous tuple-like contracts

## Testing conventions

- Each module owns its own unit tests
- Shared domain logic is tested in `shared-core/tests`
- New public state transitions or retry logic should ship with unit tests
- Tests must be runnable through `ctest`

## Logging conventions

Every log line should make it easy to correlate behavior across services.

Required contextual fields where applicable:

- `service`
- `trace_id`
- `job_id`
- `execution_id`
- `attempt`
- `worker_id`

Log levels:

- `debug` for local troubleshooting
- `info` for lifecycle milestones
- `warn` for degraded but recoverable situations
- `error` for failed operations and recovery triggers

## Config conventions

- YAML files are for local and deployment-time structure
- Secrets do not live in committed config files
- Environment variables override file values
- Each service loads only the sections it needs

## CI baseline behavior

The first CI workflow should:

1. Configure the project with `CMake`
2. Build all targets
3. Run unit tests through `ctest`
4. Run `clang-tidy` on tracked source files once code exists

The CI skeleton may initially contain placeholder steps for linting until
compilable sources are added, but the workflow structure must already exist.

## Exit criteria

- Formatting rules are committed and deterministic
- Linting rules are committed and runnable
- A unit test framework is selected and wired into the build direction
- Logging and config libraries are chosen for shared-core integration
- A CI workflow file exists with configure, build, test, and lint stages
- A new contributor can tell how code should be formatted, tested, and checked

