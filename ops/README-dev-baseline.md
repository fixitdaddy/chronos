# Developer baseline

Preferred local commands:

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default
```

Local infrastructure stack:

```bash
docker compose -f ops/compose/docker-compose.yml up -d
```

Formatting and linting baseline:

- `clang-format` using repo-root `.clang-format`
- `clang-tidy` using repo-root `.clang-tidy`

Selected libraries:

- Unit tests: `doctest`
- Logging: `spdlog`
- Config parsing: `yaml-cpp`
