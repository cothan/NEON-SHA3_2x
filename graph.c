#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <sys/random.h>
#include <string.h>
#include "fips202x2.h"
#include "fips202.h"

#define TESTS 1000000
#define OUTLENGTH 4096
#define INLENGTH 1024

/*
Compile flags:
gcc fips202x2.c fips202.c graph.c -o neon_fips202 -O3 -g3 -lpapi -mtune=native -march=native -fomit-frame-pointer -fwrapv -Wall -Wextra -Wpedantic -fno-tree-vectorize
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
    double neon, fips;

    getrandom(in1, il, GRND_NONBLOCK);
    getrandom(in2, il, GRND_NONBLOCK);
    memcpy(in_gold1, in1, il);
    memcpy(in_gold2, in2, il);

    long_long start, end;
    start = PAPI_get_real_cyc();
    for (int j = 0; j < TESTS; j++)
    {
        funcx2(out1, out2, ol, in1, in2, il);
    }
    end = PAPI_get_real_cyc();

    neon = ((double)(end - start)) / TESTS;
    // printf("NEON: %f cyc/operation\n", neon);

    start = PAPI_get_real_cyc();
    for (int j = 0; j < TESTS; j++)
    {
        func(out_gold1, ol, in_gold1, il);
        func(out_gold2, ol, in_gold2, il);
    }
    end = PAPI_get_real_cyc();

    fips = ((double)(end - start)) / TESTS;
    // printf("FIPS: %f cyc/operation\n", fips);

    *fa = neon;
    *fb = fips;

    // if (compare(out_gold1, out1, ol) && compare(out_gold2, out2, ol))
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
            printf("[%d, %d], [%lf, %lf]\n", ol, il, fa, fb);
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
            printf("[%d, %d], [%lf, %lf]\n", ol, il, fa, fb);
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
    int ret = 0;
    printf("BENCHMARK SHAKE128:\n");
    ret |= bench_shake128();
    printf("BENCHMARK SHAKE256:\n");
    ret |= bench_shake256();

    return ret;
}
