cmake_minimum_required(VERSION 3.16)
project(rayvault C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(RAYVAULT_BUILD_TESTS "Build unit tests" ON)
option(RAYVAULT_BUILD_EXAMPLES "Build examples" ON)
option(RAYVAULT_BUILD_FUZZERS "Build fuzz harnesses" OFF)

add_library(rayvault
  src/rayvault.c
  src/internal/rv_arena.c
  src/internal/rv_buf.c
  src/internal/rv_crc.c
  src/internal/rv_log.c
  src/internal/rv_diag.c
  src/internal/rv_format.c
  src/internal/rv_reader.c
  src/internal/rv_section.c
  src/internal/rv_parser.c
  src/internal/rv_dict.c
  src/internal/rv_index.c
  src/internal/rv_xref.c
  src/internal/rv_cache.c
  src/internal/rv_stream.c
  src/internal/rv_validate.c
  src/internal/rv_normalize.c
  src/internal/rv_stats.c
  src/internal/rv_export.c
  src/internal/rv_checkpoint.c
  src/internal/rv_recover.c
  src/internal/rv_legacy.c
  src/internal/rv_config.c
  src/internal/rv_notify.c
  src/internal/rv_analytics.c
  src/internal/rv_query.c
  src/internal/rv_waveproc.c
  src/internal/rv_acqmeta.c
  src/internal/rv_compat.c
  src/internal/rv_session_ops.c
  src/internal/rv_stringpool.c
  src/internal/rv_footer.c
  src/internal/rv_builder.c
  src/internal/rv_events.c
  src/internal/rv_diff.c
  src/internal/rv_merge.c
  src/internal/rv_report.c
  src/internal/rv_plugin.c
  src/internal/rv_filter.c
  src/internal/rv_traceutil.c
  src/internal/rv_plant.c
  src/internal/rv_textexport.c
  src/internal/rv_otdrmath.c
  src/internal/rv_batch.c
)

target_include_directories(rayvault
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
)

target_compile_options(rayvault PRIVATE
  $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wno-unused-parameter>
)

if(RAYVAULT_BUILD_TESTS)
  enable_testing()
  add_executable(test_package tests/test_package.c)
  target_link_libraries(test_package PRIVATE rayvault)
  target_include_directories(test_package PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
  add_test(NAME test_package COMMAND test_package)

  add_executable(test_lifecycle tests/test_lifecycle.c)
  target_link_libraries(test_lifecycle PRIVATE rayvault)
  target_include_directories(test_lifecycle PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
  add_test(NAME test_lifecycle COMMAND test_lifecycle)

  add_executable(test_stream_cache tests/test_stream_cache.c)
  target_link_libraries(test_stream_cache PRIVATE rayvault)
  target_include_directories(test_stream_cache PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
    ${CMAKE_CURRENT_SOURCE_DIR}/src)
  add_test(NAME test_stream_cache COMMAND test_stream_cache)
endif()

if(RAYVAULT_BUILD_EXAMPLES)
  add_executable(rv_inspect examples/rv_inspect.c)
  target_link_libraries(rv_inspect PRIVATE rayvault)
  target_include_directories(rv_inspect PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/internal)

  add_executable(rv_pack examples/rv_pack.c)
  target_link_libraries(rv_pack PRIVATE rayvault)
  target_include_directories(rv_pack PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/internal)
endif()

if(RAYVAULT_BUILD_FUZZERS)
  set(FUZZ_ENGINE "$ENV{LIB_FUZZING_ENGINE}")
  if(NOT FUZZ_ENGINE)
    set(FUZZ_ENGINE "-fsanitize=fuzzer")
  endif()
  foreach(harness
      package_session_fuzzer
      span_query_fuzzer
      wave_stream_fuzzer
      dataset_export_fuzzer
      repair_reopen_fuzzer)
    add_executable(${harness} fuzz/${harness}.c)
    target_link_libraries(${harness} PRIVATE rayvault ${FUZZ_ENGINE})
    target_include_directories(${harness} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src/internal
      ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_options(${harness} PRIVATE -fsanitize=fuzzer,address,undefined)
    target_link_options(${harness} PRIVATE -fsanitize=fuzzer,address,undefined)
  endforeach()
endif()

install(TARGETS rayvault ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(DIRECTORY include/rayvault DESTINATION include)
