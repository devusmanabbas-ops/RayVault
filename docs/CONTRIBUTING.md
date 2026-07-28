# Contributing

## Coding style

- C11, two-space or four-space indentation as already used in the file
- Prefer clear ownership comments at subsystem boundaries
- Public headers stay under `include/rayvault/`
- Internal headers stay under `src/internal/`

## Testing

Every behavioral change should update or add a test under `tests/`.

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Pull requests

- Keep commits focused
- Do not add network-dependent build steps
- Update `docs/format.md` when on-disk layout changes
- When adding a fuzz harness, register it in `.clusterfuzzlite/project.yaml`
  and `.clusterfuzzlite/build.sh`

## Release checklist

1. Bump `rv_version_string`
2. Confirm format minor/major notes in compatibility docs
3. Rebuild corpus seeds if section layouts changed
