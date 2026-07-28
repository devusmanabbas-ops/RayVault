# Compatibility notes

## Format versions

| Major | Status | Notes |
|-------|--------|-------|
| 2 | Current | Full section set, SUMM footer, feature bits |
| 1 | Legacy | Accepted with `RV_OPEN_ALLOW_LEGACY`; migrate via `rv_package_migrate_legacy` |
| 0 | Rejected | Pre-release prototypes |

## Deprecated APIs

The `rv_compat_*` helpers wrap the session API for 1.x call sites. New code
should use `rv_package_*` and `rv_session_*` directly.

Macros `rayvault_open_file` / `rayvault_close` remain as aliases.

## Migration behavior

`rv_package_migrate_legacy` rewrites the package backing buffer into a v2
image. Parsed waveform views may continue referencing prior storage until the
caller resets the package or rebinds the session. Monitoring UIs that keep a
warm cache across format bumps should call `rv_session_rebind` after migration.

## Feature bits

| Bit | Meaning |
|-----|---------|
| 0x100 | Package produced by legacy migration |
