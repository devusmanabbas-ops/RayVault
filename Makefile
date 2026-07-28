# RayVault Makefile (fallback when CMake is unavailable)
CC ?= cc
AR ?= ar
CFLAGS ?= -O2 -g -Wall -Wextra -Wno-unused-parameter -std=c11
CPPFLAGS += -Iinclude -Isrc -Isrc/internal
LDFLAGS ?=

LIB = librayvault.a
SRCS = \
  src/rayvault.c \
  src/internal/rv_arena.c \
  src/internal/rv_buf.c \
  src/internal/rv_crc.c \
  src/internal/rv_log.c \
  src/internal/rv_diag.c \
  src/internal/rv_format.c \
  src/internal/rv_reader.c \
  src/internal/rv_section.c \
  src/internal/rv_parser.c \
  src/internal/rv_dict.c \
  src/internal/rv_index.c \
  src/internal/rv_xref.c \
  src/internal/rv_cache.c \
  src/internal/rv_stream.c \
  src/internal/rv_validate.c \
  src/internal/rv_normalize.c \
  src/internal/rv_stats.c \
  src/internal/rv_export.c \
  src/internal/rv_checkpoint.c \
  src/internal/rv_recover.c \
  src/internal/rv_legacy.c \
  src/internal/rv_config.c \
  src/internal/rv_notify.c \
  src/internal/rv_analytics.c \
  src/internal/rv_query.c \
  src/internal/rv_waveproc.c \
  src/internal/rv_acqmeta.c \
  src/internal/rv_compat.c \
  src/internal/rv_session_ops.c \
  src/internal/rv_stringpool.c \
  src/internal/rv_footer.c \
  src/internal/rv_builder.c \
  src/internal/rv_events.c \
  src/internal/rv_diff.c \
  src/internal/rv_merge.c \
  src/internal/rv_report.c \
  src/internal/rv_plugin.c \
  src/internal/rv_filter.c \
  src/internal/rv_traceutil.c \
  src/internal/rv_plant.c \
  src/internal/rv_textexport.c \
  src/internal/rv_otdrmath.c \
  src/internal/rv_batch.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean test examples fuzz-objs

all: $(LIB) examples test

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

examples: rv_inspect rv_pack

rv_inspect: examples/rv_inspect.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/rv_inspect.c $(LIB) -o $@ $(LDFLAGS) -lm

rv_pack: examples/rv_pack.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/rv_pack.c $(LIB) -o $@ $(LDFLAGS) -lm

test: test_package test_lifecycle test_stream_cache
	./test_package
	./test_lifecycle
	./test_stream_cache

test_package: tests/test_package.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_package.c $(LIB) -o $@ $(LDFLAGS) -lm

test_lifecycle: tests/test_lifecycle.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_lifecycle.c $(LIB) -o $@ $(LDFLAGS) -lm

test_stream_cache: tests/test_stream_cache.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_stream_cache.c $(LIB) -o $@ $(LDFLAGS) -lm

clean:
	rm -f $(OBJS) $(LIB) rv_inspect rv_pack test_package test_lifecycle test_stream_cache
	rm -f fuzz/*.o
