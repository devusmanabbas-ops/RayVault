# Fuzzing guide

RayVault ships five libFuzzer harnesses that exercise distinct product
lifecycles:

| Harness | Focus |
|---------|-------|
| `package_session_fuzzer` | Open, validate, session, warm cache |
| `span_query_fuzzer` | Index queries, plant report, rebuild |
| `wave_stream_fuzzer` | Stream/cursor with small LRU cache |
| `dataset_export_fuzzer` | Derived summary, checkpoint, export |
| `repair_reopen_fuzzer` | Repair, legacy migrate, rebind, re-read |

## Local build

```bash
export CC=clang CXX=clang++
cmake -S . -B build-fuzz -DRAYVAULT_BUILD_FUZZERS=ON
cmake --build build-fuzz
```

## Seeds

```bash
python3 scripts/prepare_corpus.py
```

Shared seeds live in `corpus/`. Per-harness copies are under
`fuzz/corpus/<harness>/`. Dictionary: `fuzz/rayvault.dict`.

## ClusterFuzzLite

`.clusterfuzzlite/build.sh` builds every harness into `$OUT` without network
access. `project.yaml` lists all five targets.

Example local invocation:

```bash
./build-fuzz/wave_stream_fuzzer fuzz/corpus/wave_stream_fuzzer -dict=fuzz/rayvault.dict -max_len=65536
```
