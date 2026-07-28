# Build guide

## Requirements

- CMake 3.16+
- A C11 compiler (GCC, Clang, or Apple Clang)
- No network access is required; the tree is fully offline

## Configure and build

```bash
cmake -S . -B build
cmake --build build
```

Useful options:

| Option | Default | Description |
|--------|---------|-------------|
| `RAYVAULT_BUILD_TESTS` | ON | Unit tests |
| `RAYVAULT_BUILD_EXAMPLES` | ON | `rv_inspect`, `rv_pack` |
| `RAYVAULT_BUILD_FUZZERS` | OFF | libFuzzer harnesses |

## Sanitizer build

```bash
cmake -S . -B build-asan \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan
```

## Fuzz harnesses

See [fuzzing.md](fuzzing.md). ClusterFuzzLite uses `.clusterfuzzlite/build.sh`.

## Install

```bash
cmake --install build --prefix /usr/local
```
