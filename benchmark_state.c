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
#include <papi.h>
#include <time.h>
#include "fips202x2.h"
#include "fips202.h"

#define TESTS 1000
#define OUTLENGTH 4096
#define INLENGTH 4096
#define STATE_SIZE (64 / 8 * 25)

/*
Compile flags:
gcc -o bench_state_neon_fips202 fips202x2.c fips202.c benchmark_state.c -O3 -g3 -lpapi -mtune=native -march=native -fomit-frame-pointer -fwrapv -Wall -Wextra -Wpedantic -fno-tree-vectorize
*/

int compare(uint8_t *out_gold, uint8_t *out1, uint8_t *out2, int ol)
{
    int check = 0;
    uint8_t m, n;

    for (int i = 0; i < ol - 8; i += 8)
    {
        for (int j = i; j < i + 8; j++)
        {
            m = out_gold[j];
            n = out1[j];
            if (out_gold[j] != out1[j])
            {
                printf("![%2d] %2x != %2x  ?? %2x\n", j, m, n, out2[j]);
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
          uint8_t *out_gold, uint8_t *out1, uint8_t *out2, int ol,
          uint8_t *in_gold, uint8_t *in1, uint8_t *in2, int il)
{
    uint64_t a, b;

    for (int i = 0; i < il; i++)
    {
        a = rand();
        b = rand();
        a = (a << 32) | rand();
        b = (b << 32) | rand();
        in1[i] = a;
        in2[i] = b;
        in_gold[i] = a;
    }

    long_long start, end;
    start = PAPI_get_real_cyc();
    for (int j = 0; j < TESTS; j++)
    {
        funcx2(out1, out2, ol, in1, in2, il);
    }
    end = PAPI_get_real_cyc();
    printf("NEON: %f cyc/operation\n", ((double)(end - start)) / TESTS);

    start = PAPI_get_real_cyc();
    for (int j = 0; j < TESTS; j++)
    {
        func(out_gold, ol, in_gold, il);
    }
    end = PAPI_get_real_cyc();
    printf("FIPS: %f cyc/operation\n", ((double)(end - start)) / TESTS);

    if (compare(out_gold, out1, out2, ol))
        return 1;

    return 0;
}

int bench_shake128()
{
    uint8_t out1[OUTLENGTH], out2[OUTLENGTH];
    uint8_t in1[INLENGTH], in2[INLENGTH];
    uint8_t out_gold[OUTLENGTH], in_gold[INLENGTH];

    int ret;
    void (*func)() = &shake128,
         (*funcx2)() = &shake128x2;

    for (int ol = STATE_SIZE; ol < OUTLENGTH; ol += STATE_SIZE)
    {
        for (int il = 0; STATE_SIZE < INLENGTH; il += STATE_SIZE)
        {
            ret = bench(func, funcx2, out_gold, out1, out2, ol, in_gold, in1, in2, il);
            printf("[%d, %d]\n", ol, il);
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
    uint8_t out_gold[OUTLENGTH], in_gold[INLENGTH];

    int ret;
    void (*func)() = &shake256,
         (*funcx2)() = &shake256x2;

    for (int ol = STATE_SIZE; ol < OUTLENGTH; ol += STATE_SIZE)
    {
        for (int il = STATE_SIZE; il < INLENGTH; il += STATE_SIZE)
        {
            ret = bench(func, funcx2, out_gold, out1, out2, ol, in_gold, in1, in2, il);
            printf("[%d, %d]\n", ol, il);
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
