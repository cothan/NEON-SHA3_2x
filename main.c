#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <time.h>
#include "fips202x2.h"
#include "fips202.h"

#define TESTS 1000000
#define OUTLENGTH 4096
#define INLENGTH 4096
#define BRUTE 1

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
           uint8_t *in_gold, uint8_t *in1, uint8_t *in2, int il,
           const char *string)
{
    uint64_t a, b;

    // printf("%s:\n", string);
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
    // printf("NEON: %lld/operation\n", (end - start)/TESTS);
    printf("NEON: %f cyc/operation\n", ((double)(end - start)) / TESTS);

    start = PAPI_get_real_cyc();
    for (int j = 0; j < TESTS; j++)
    {
        func(out_gold, ol, in_gold, il);
    }
    end = PAPI_get_real_cyc();
    // printf("FIPS: %lld/operation\n", (end - start)/TESTS);
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
    void (*func)()   = &shake128, 
         (*funcx2)() = &shake128x2;

    for (int ol = 1; ol < OUTLENGTH; ol++)
    {
        for (int il = 1; il < INLENGTH; il++)
        {
            ret = bench(func, funcx2, out_gold, out1, out2, ol, in_gold, in1, in2, il, "BENCHMARK SHAKE128");
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
    void (*func)()   = &shake256, 
        (*funcx2)() = &shake256x2;

    for (int ol = 1; ol < OUTLENGTH; ol++)
    {
        for (int il = 1; il < INLENGTH; il++)
        {
            ret = bench(func, funcx2, out_gold, out1, out2, ol, in_gold, in1, in2, il, "BENCHMARK SHAKE256");
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
