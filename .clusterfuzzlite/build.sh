#!/bin/bash
# ClusterFuzzLite build script for RayVault fuzz targets.
# Expected environment: $SRC (repo root), $OUT (artifact dir), $CXX/$CC,
# optional $CFLAGS/$CXXFLAGS/$LIB_FUZZING_ENGINE.
set -eu

cd "$SRC"

# Prefer clang when available; fall back to environment compiler.
if command -v clang >/dev/null 2>&1; then
  export CC="${CC:-clang}"
  export CXX="${CXX:-clang++}"
fi

FUZZ_CFLAGS="${CFLAGS:-} -g -O1"
FUZZ_ENGINE="${LIB_FUZZING_ENGINE:--fsanitize=fuzzer}"
INCLUDES="-Iinclude -Isrc -Isrc/internal"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

if command -v cmake >/dev/null 2>&1; then
  mkdir -p build-cfl
  cd build-cfl
  cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DRAYVAULT_BUILD_TESTS=OFF \
    -DRAYVAULT_BUILD_EXAMPLES=OFF \
    -DRAYVAULT_BUILD_FUZZERS=OFF
  cmake --build . --target rayvault -j"$JOBS"
  cd "$SRC"
  LIB="build-cfl/librayvault.a"
else
  # Fallback for environments without CMake.
  make -j"$JOBS" librayvault.a
  LIB="librayvault.a"
fi

build_one() {
  local name="$1"
  echo "Building $name"
  $CC $FUZZ_CFLAGS -fsanitize=fuzzer-no-link,address,undefined \
    $INCLUDES -c "fuzz/${name}.c" -o "build-cfl/${name}.o"
  $CXX $FUZZ_CFLAGS -fsanitize=address,undefined \
    "build-cfl/${name}.o" $LIB $FUZZ_ENGINE -o "$OUT/${name}"
}

mkdir -p "$OUT"
build_one package_session_fuzzer
build_one span_query_fuzzer
build_one wave_stream_fuzzer
build_one dataset_export_fuzzer
build_one repair_reopen_fuzzer

# Seed corpora
for name in package_session_fuzzer span_query_fuzzer wave_stream_fuzzer \
            dataset_export_fuzzer repair_reopen_fuzzer; do
  if [ -d "fuzz/corpus/${name}" ]; then
    mkdir -p "$OUT/${name}_seed_corpus"
    cp -r fuzz/corpus/${name}/* "$OUT/${name}_seed_corpus/" 2>/dev/null || true
  fi
done

if [ -d corpus ]; then
  mkdir -p "$OUT/shared_seed_corpus"
  cp -r corpus/* "$OUT/shared_seed_corpus/" 2>/dev/null || true
fi

if [ -f fuzz/rayvault.dict ]; then
  cp fuzz/rayvault.dict "$OUT/rayvault.dict"
fi

echo "ClusterFuzzLite build complete"
