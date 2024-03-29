CC ?= /usr/bin/gcc
CFLAGS += -O3 -mtune=native -fomit-frame-pointer -fwrapv -Wall -Wextra -Wpedantic -fno-tree-vectorize
RM = /bin/rm

SOURCES = fips202x2.c fips202.c
HEADERS = fips202x2.h fips202.h

.PHONY: all shared clean

all: \
	bench_rate_neon_fips202 \
	benchmark_mem \
	benchmark

shared: \
	libsha3x2_neon.so \
	libsha3.so

bench_rate_neon_fips202: fips202x2.c fips202.c benchmark_rate.c
	$(CC) $(CFLAGS) $(SOURCES) benchmark_rate.c -o bench_rate_neon_fips202

bench:
	./benchmark
	./benchmark_mem

benchmark_mem: fips202x2.c fips202.c benchmark.cxx
	c++ $(SOURCES) benchmark.cxx -DMEM=1 -o $@ -I/usr/local/include -L/usr/local/lib -lbenchmark -std=c++11  -O3

benchmark: fips202x2.c fips202.c benchmark.cxx
	c++ $(SOURCES) benchmark.cxx -DMEM=0 -o $@ -I/usr/local/include -L/usr/local/lib -lbenchmark -std=c++11  -O3

libsha3x2_neon.so: fips202x2.c fips202x2.h
	$(CC) -shared -fPIC $(CFLAGS) fips202x2.c -o libsha3x2_neon.so

libsha3.so: fips202.c fips202.h
	$(CC) -shared -fPIC $(CFLAGS) fips202.c -o libsha3.so

clean:
	-$(RM) -rf *.gcno *.gcda *.lcov *.o *.so
	-$(RM) -rf bench_rate_neon_fips202
	-$(RM) -rf benchmark
	-$(RM) -rf libsha3x2_neon.so
	-$(RM) -rf libsha3.so



