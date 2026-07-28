# API overview

Include `rayvault/rayvault.h`.

## Packages

- `rv_package_open_file` / `rv_package_open_memory`
- `rv_package_reopen` / `rv_package_reset` / `rv_package_close`
- `rv_package_get_info` / `rv_package_validate`
- `rv_package_repair` / `rv_package_migrate_legacy`

## Sessions

A session binds indexes, cache, and derived stats to an open package.

- `rv_session_create` / `rv_session_destroy`
- `rv_session_rebuild_index` / `rv_session_warm_cache`
- `rv_session_stats` / `rv_session_set_notify`

## Queries

- `rv_name_lookup` / `rv_name_find`
- `rv_route_count` / `rv_route_at`
- `rv_instrument_count` / `rv_instrument_at`
- `rv_window_count` / `rv_window_find` / `rv_window_get`
- `rv_marker_query` / `rv_wave_get`

## Streaming

- `rv_stream_open` → `rv_stream_next_window` / `rv_stream_next_wave`
- `rv_cursor_open` → `rv_cursor_seek_window` / `rv_cursor_read_wave`

## Export and checkpoints

- `rv_checkpoint_create` / `rv_checkpoint_restore`
- `rv_export_buffer` / `rv_export_free`

## Configuration

Call `rv_config_init` then adjust arena and cache sizes before open:

```c
rv_config cfg;
rv_config_init(&cfg);
cfg.arena_initial_bytes = 64 * 1024;
cfg.cache_capacity = 8;
rv_package_open_file(&pkg, path, RV_OPEN_BUILD_INDEX, &cfg);
```
