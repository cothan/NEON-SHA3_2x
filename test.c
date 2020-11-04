#include <arm_neon.h>
#include <stdio.h>

#define ROL(a, offset) ((a << offset) ^ (a >> (64-offset)))

#define vload(ptr) vld1q_u64(ptr);

#define vstore(ptr, c) vst1q_u64(ptr, c);

#define vROL(a, t, offset) \
    t = vshlq_n_u64(a, offset);\
    a = vsriq_n_u64(t, a, 64-offset);

#define offset 16

int main()
{
    uint64_t a, a_test[2] = {0xAAAAfcfdaabbccdd, 0xAAAAfcfdaabbccdd};
    a = 0xAAAAfcfdaabbccdd;
    
    uint64x2_t vector, t;

    printf("a: %lx\n", a);
    a = ROL(a, offset);
    printf("a: %lx\n", a);
    
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

    vector = vload(a_test);
    vROL(vector, t, offset);
    vstore(a_test, vector);
    printf("a_test: %lx\n", a_test[0]);
    printf("a_test: %lx\n", a_test[1]);

    return 0;
}

// https://pcpartpicker.com/list/Y3bXbh