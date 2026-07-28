# RayVault

RayVault is a C library for reading, indexing, streaming, validating, and
exporting optical fiber OTDR monitoring datasets stored in the RVP package
format.

Field acquisition systems and plant monitoring tools produce RVP packages
that bundle fiber route inventories, instrument calibrations, acquisition
windows, waveform samples, and event markers. RayVault gives offline
analysis pipelines a stable API over that data without requiring a live
connection to the acquisition head-end.

## Features

- Versioned RVP package reader with section directory and CRC checks
- Session-scoped indexes for routes, windows, markers, and wave blocks
- Waveform cache with LRU eviction for multi-window replay
- Stream and cursor APIs for plant walkthroughs
- Export / checkpoint / repair / legacy migration helpers
- Offline builders for packaging synthetic or converted datasets

## Quick start

```bash
# Make (works offline with system cc)
make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc) all

# Or CMake
cmake -S . -B build -DRAYVAULT_BUILD_TESTS=ON -DRAYVAULT_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure

./rv_pack demo.rvp
./rv_inspect demo.rvp
```

## Documentation

- [Build guide](docs/build.md)
- [Architecture](docs/architecture.md)
- [API overview](docs/api.md)
- [RVP file format](docs/format.md)
- [Compatibility](docs/compatibility.md)
- [Fuzzing](docs/fuzzing.md)
- [Examples](docs/examples.md)
- [Contributing](docs/CONTRIBUTING.md)

## License

Proprietary — internal use. Contact the maintainers for redistribution terms.
