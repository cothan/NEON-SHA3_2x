CC ?= /usr/bin/gcc
CFLAGS += -O3 -mtune=native -march=native -fomit-frame-pointer -fwrapv -Wall -Wextra -Wpedantic -fno-tree-vectorize -lpapi
RM = /bin/rm

SOURCES = fips202x2.c fips202.c
HEADERS = fips202x2.h fips202.h

.PHONY: all shared clean

all: \
	bench_rate_neon_fips202 \
	bench_state_neon_fips202

shared: \
	libsha3x2_neon.so \
	libsha3.so

bench_rate_neon_fips202: fips202x2.c fips202.c benchmark_rate.c
	$(CC) $(CFLAGS) $(SOURCES) benchmark_rate.c -o bench_rate_neon_fips202

bench_state_neon_fips202: fips202x2.c fips202.c benchmark_state.c
	$(CC) $(CFLAGS) $(SOURCES) benchmark_state.c -o bench_state_neon_fips202

libsha3x2_neon.so: fips202x2.c fips202x2.h
	$(CC) -shared -fPIC $(CFLAGS) fips202x2.c -o libsha3x2_neon.so

libsha3.so: fips202.c fips202.h
	$(CC) -shared -fPIC $(CFLAGS) fips202.c -o libsha3.so

clean:
	-$(RM) -rf *.gcno *.gcda *.lcov *.o *.so
	-$(RM) -rf bench_rate_neon_fips202
	-$(RM) -rf bench_state_neon_fips202
	-$(RM) -rf libsha3x2_neon.so
	-$(RM) -rf libsha3.so



