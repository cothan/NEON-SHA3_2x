#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <time.h>
#include "fips202x2.h"
#include "fips202.h"

#define TESTS 1
#define OUTLENGTH 100
#define INLENGTH 123

int main()
{
    // srand(time(0));
    uint8_t out1[OUTLENGTH], out2[OUTLENGTH];
    uint8_t in1[INLENGTH], in2[INLENGTH];
    uint8_t out_gold[OUTLENGTH], in_gold[INLENGTH];

    uint64_t bla, blo;

    for (int i = 0; i < INLENGTH; i++)
    {
        bla = rand();
        blo = rand();
        in1[i] = bla;
        in2[i] = blo;
        in_gold[i] = bla;
        // in_gold[i] = blo;
    }
    long_long start, end;

    start = PAPI_get_real_cyc();
    for (int j = 0; j < TESTS; j++)
    {
        shake128x2(out1, out2, OUTLENGTH, in1, in2, INLENGTH);
    }
    end = PAPI_get_real_cyc();
    printf("NEON: %lld\n", end - start);

    start = PAPI_get_real_cyc();
    for (int j = 0; j < TESTS; j++)
    {
        shake128(out_gold, OUTLENGTH, in_gold, INLENGTH);
    }
    end = PAPI_get_real_cyc();
    printf("FIPS: %lld\n", end - start);

    int check = 0;
    uint8_t m, n;

    for (int i = 0; i < OUTLENGTH; i += 8)
    {
        for (int j = i; j < i + 8; j++)
        {
            m = out_gold[j];
            n = out1[j];
            if (out_gold[j] != out1[j])
            {
                printf("![%2d] %2x != %2x\n", j,  m, n);
                check++;
            }
            else
            {
                printf("+[%2d] %2x == %2x\n", j,  m, n);
            }
            
        }

        if (check == 8)
        {
            printf("ERROR! %d\n", i);
            return 1;
        }
    }

    printf("CORRECT!\n");

    return 0;
}
