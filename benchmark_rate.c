/*=============================================================================
 * Copyright (c) 2020 by Cryptographic Engineering Research Group (CERG)
 * ECE Department, George Mason University
 * Fairfax, VA, U.S.A.
 * Author: Duc Tri Nguyen
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
=============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fips202x2.h"
#include "fips202.h"

#define TESTS 1000
#define OUTLENGTH 4096
#define INLENGTH 1024

#define VERBOSE 0

#define TIME(s) clock_gettime(CLOCK_MONOTONIC_RAW, &s);
#define CALC(start, stop, ntests) ((double)((stop.tv_sec - start.tv_sec) * 1000000000 + (stop.tv_nsec - start.tv_nsec))) / ntests

/*
Compile flags:
gcc -o bench_rate_neon_fips202 fips202x2.c fips202.c benchmark_rate.c -O3 -lpapi -mtune=native -march=native -fomit-frame-pointer -fwrapv -Wall -Wextra -Wpedantic -fno-tree-vectorize
*/

int compare(uint8_t *out_gold, uint8_t *out, int ol)
{
    int check = 0;
    uint8_t m, n;

    for (int i = 0; i < ol - 8; i += 8)
    {
        for (int j = i; j < i + 8; j++)
        {
            m = out_gold[j];
            n = out[j];
            if (out_gold[j] != out[j])
            {
                printf("![%2d] %2x != %2x\n", j, m, n);
                check++;
            }
        }

        if (check)
        {
            printf("%d ERROR! %d\n", ol, i);
            return 1;
        }
    }
    return 0;
}

int bench(void func(), void funcx2(),
          uint8_t *out_gold1, uint8_t *out_gold2,
          uint8_t *out1, uint8_t *out2, int ol,
          uint8_t *in_gold1, uint8_t *in_gold2,
          uint8_t *in1, uint8_t *in2, int il,
          double *fa, double *fb)
{
    double neon = 0, fips = 0;
    double neon_ns = 0, fips_ns = 0;

    for (int i = 0; i < il; i++)
    {
        in1[i] = rand() & 0xff;
        in2[i] = rand() & 0xff;
    }
    memcpy(in_gold1, in1, il);
    memcpy(in_gold2, in2, il);

    struct timespec start, end;
    TIME(start);
    for (int j = 0; j < TESTS; j++)
    {
        funcx2(out1, out2, ol, in1, in2, il);
    }
    TIME(end);
    neon_ns = CALC(start, end, TESTS);
    if (VERBOSE) printf("NEON: %f ns/operation\n", neon_ns);

    TIME(start);
    for (int j = 0; j < TESTS; j++)
    {
        func(out_gold1, ol, in_gold1, il);
        func(out_gold2, ol, in_gold2, il);
    }
    TIME(end);
    fips_ns = CALC(start, end, TESTS);
    if (VERBOSE) printf("FIPS: %f ns/operation. Ratio %.2f\n", fips_ns, fips_ns/neon_ns);

    *fa = neon_ns;
    *fb = fips_ns;

    if (memcmp(out_gold1, out1, ol) || memcmp(out_gold2, out2, ol))
        return 1;

    return 0;
}

int bench_shake128()
{
    uint8_t out1[OUTLENGTH], out2[OUTLENGTH];
    uint8_t in1[INLENGTH], in2[INLENGTH];
    uint8_t out_gold1[OUTLENGTH], in_gold1[INLENGTH],
            out_gold2[OUTLENGTH], in_gold2[INLENGTH];
    double fa, fb;

    int ret;
    void (*func)() = &shake128,
         (*funcx2)() = &shake128x2;

    for (int ol = SHAKE128_RATE / 4; ol <= OUTLENGTH; ol += SHAKE128_RATE / 4)
    {
        for (int il = SHAKE128_RATE / 4; il <= INLENGTH; il += SHAKE128_RATE / 4)
        {
            ret = bench(func, funcx2, out_gold1, out_gold2, out1, out2, ol, in_gold1, in_gold2, in1, in2, il, &fa, &fb);
            if (!VERBOSE) printf("[%d, %d], [%lf, %lf]\n", ol, il, fa, fb);
            if (ret)
            {
                printf("%d %d: ERROR\n", ol, il);
                return 1;
            }
        }
    }
    printf("ALL CORRECT!\n");
    printf("============\n");

    return 0;
}

int bench_shake256()
{
    uint8_t out1[OUTLENGTH], out2[OUTLENGTH];
    uint8_t in1[INLENGTH], in2[INLENGTH];
    uint8_t out_gold1[OUTLENGTH], in_gold1[INLENGTH],
            out_gold2[OUTLENGTH], in_gold2[INLENGTH];
    double fa, fb;

    int ret;
    void (*func)() = &shake256,
         (*funcx2)() = &shake256x2;

    for (int ol = SHAKE256_RATE / 4; ol <= OUTLENGTH; ol += SHAKE256_RATE / 4)
    {
        for (int il = SHAKE256_RATE / 4; il <= INLENGTH; il += SHAKE256_RATE / 4)
        {
            ret = bench(func, funcx2, out_gold1, out_gold2, out1, out2, ol, in_gold1, in_gold2, in1, in2, il, &fa, &fb);
            if (!VERBOSE) printf("[%d, %d], [%lf, %lf]\n", ol, il, fa, fb);
            if (ret)
            {
                printf("%d %d: ERROR\n", ol, il);
                return 1;
            }
        }
    }
    printf("ALL CORRECT!\n");
    printf("============\n");

    return 0;
}

int main()
{
    srand(time(0));
    int ret = 0;
    printf("BENCHMARK SHAKE128:\n");
    ret |= bench_shake128();
    printf("BENCHMARK SHAKE256:\n");
    ret |= bench_shake256();

    return ret;
}


