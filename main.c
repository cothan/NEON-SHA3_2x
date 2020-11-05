#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <time.h>

#define TESTS 10000000
#define ROL(a, offset) ((a << offset) ^ (a >> (64-offset)))


int main()
{
    v128 state[25];
    uint64_t s[25];
    v128 t;
    uint64_t bla, blo;
    srand(time(0));

    for (int i  =0; i < 25; i++)
    {
        bla = rand();
        blo = rand();
        s[i] = bla ^ blo;
        t = vdupq_n_u64(bla ^ blo);
        state[i] = t;
    }
    long_long start, end;

    start = PAPI_get_real_cyc();
    for (int j = 0; j < TESTS; j++){
        neon_KeccakF1600_StatePermute(state);
    }
    end = PAPI_get_real_cyc();
    printf("NEON: %lld\n", end - start);

    start = PAPI_get_real_cyc();
    for (int j = 0; j < TESTS; j++)
    {
        KeccakF1600_StatePermute(s);
    }
    end = PAPI_get_real_cyc();
    printf("FIPS: %lld\n", end - start);
        

    v128 sum = vdupq_n_u64(0);
    uint64_t ssum = 0;
    for (int i = 0; i < 25; i++)
    {
        sum += state[i];
        ssum += s[i];
    }

    if (sum[1] != ssum)
    {
        printf("=====\n");
        printf("%lx\n", sum[1]);
        printf("%lx\n", ssum);
        return 1;
    }

    return 0;
}
