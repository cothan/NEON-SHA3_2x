#include <arm_neon.h>
#include <stdio.h>

#define ROL(a, offset) ((a << offset) ^ (a >> (64-offset)))

#define vload(ptr) vld1q_u64(ptr);

#define vstore(ptr, c) vst1q_u64(ptr, c);

#define vROL(a, t, offset) \
    t = vshlq_n_u64(a, offset);\
    a = vsriq_n_u64(t, a, 64-offset);

#define offset 16

static uint64_t load64(const uint8_t x[8]) {
  unsigned int i;
  uint64_t r = 0;

  for(i=0;i<8;i++)
    r |= (uint64_t)x[i] << 8*i;

  return r;
}

int main()
{
    // uint64_t a, a_test[2] = {0xAAAAfcfdaabbccdd, 0xAAAAfcfdaabbccdd};
    // a = 0xAAAAfcfdaabbccdd;
    
    // uint64x2_t vector, t;

    // printf("a: %lx\n", a);
    // a = ROL(a, offset);
    // printf("a: %lx\n", a);
    
    // vector = vload(a_test);
    // vector = vector << 4;
    // vector2  = vshrq_n_u64(vector, 60);
    // vsliq_n_u64(vector2, vector, 4);

    // a << offset 
    // vector2 = vshlq_n_u64(vector, offset);
    // vstore(a_test, vector2);
    // printf("a_test: %lx\n", a_test[0]);
    // (a << offset) | (a >> (64-offset))
    // vector = vsriq_n_u64(vector2, vector, 64-offset);

    // vector = vload(a_test);
    // // vector[0] = (uint64_t) 0;
    // // vector[1] = (uint64_t) 1;
    // vROL(vector, t, offset);
    // vstore(a_test, vector);
    // printf("a_test: %lx\n", a_test[0]);
    // printf("a_test: %lx\n", a_test[1]);

    // vector[0] = 0;
    // vector[1] = 1;
    // vstore(a_test, vector);
    // printf("a_test: %lx\n", a_test[1]);
    // printf("a_test: %lx\n", a_test[0]);

    uint8_t a[16] = {0xAA, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

    uint64x1_t tmp1 = vld1_u64((uint64_t *)&a[0]);
    uint64x1_t tmp2 = vld1_u64((uint64_t *)&a[8]);

    printf("N: %lx\n", tmp1[0]);
    printf("N: %lx\n", tmp2[0]);

    printf("C: %lx\n", load64(&a[0]));
    printf("C: %lx\n", load64(&a[8]));

    return 0;
}

// https://pcpartpicker.com/list/Y3bXbh