#include <arm_neon.h>

#define NROUNDS 24

// Define NEON operation

// c = load(ptr)
#define vload(ptr) vld1q_u64(ptr);
// ptr <= c;
#define vstore(ptr, c) vst1q_u64(ptr, c);
// c = a ^ b
#define vxor(c, a, b) c = veorq_u64(a, b);
// Rotate by n bit ((a << offset) ^ (a >> (64-offset)))
#define vROL(out, a, t, offset) \
    t = vshlq_n_u64(a, offset); \
    out = vsriq_n_u64(t, a, 64 - offset);
// Xor chain: out = a ^ b ^ c ^ d ^ e
#define vXOR4(out, a, b, c, d, e) \
    out = veorq_u64(a, veorq_u64(b, veorq_u64(c, veorq_u64(d, e))));
// Not And c = ~a & b
// #define vbic(c, a, b) c = vbicq_u64(b, a);
// Xor Not And: out = a ^ ( (~b) & c)
#define vXNA(out, a, b, c) out = veorq_u64(a, vbicq_u64(b, c));

// Rotate by 1 bit, then XOR: a ^ ROL(b)
#define vrxor(c, a, b) c = vrax1q_u64(a, b);
// End Define

/* Keccak round constants */
static const uint64_t KeccakF_RoundConstants[NROUNDS] = {
    (uint64_t)0x0000000000000001ULL,
    (uint64_t)0x0000000000008082ULL,
    (uint64_t)0x800000000000808aULL,
    (uint64_t)0x8000000080008000ULL,
    (uint64_t)0x000000000000808bULL,
    (uint64_t)0x0000000080000001ULL,
    (uint64_t)0x8000000080008081ULL,
    (uint64_t)0x8000000000008009ULL,
    (uint64_t)0x000000000000008aULL,
    (uint64_t)0x0000000000000088ULL,
    (uint64_t)0x0000000080008009ULL,
    (uint64_t)0x000000008000000aULL,
    (uint64_t)0x000000008000808bULL,
    (uint64_t)0x800000000000008bULL,
    (uint64_t)0x8000000000008089ULL,
    (uint64_t)0x8000000000008003ULL,
    (uint64_t)0x8000000000008002ULL,
    (uint64_t)0x8000000000000080ULL,
    (uint64_t)0x000000000000800aULL,
    (uint64_t)0x800000008000000aULL,
    (uint64_t)0x8000000080008081ULL,
    (uint64_t)0x8000000000008080ULL,
    (uint64_t)0x0000000080000001ULL,
    (uint64_t)0x8000000080008008ULL};

typedef uint64x2_t v128;

static void KeccakF1600_StatePermute(v128 state[25])
{
    v128 Aba, Abe, Abi, Abo, Abu;
    v128 Aga, Age, Agi, Ago, Agu;
    v128 Aka, Ake, Aki, Ako, Aku;
    v128 Ama, Ame, Ami, Amo, Amu;
    v128 Asa, Ase, Asi, Aso, Asu;
    v128 BCa, BCe, BCi, BCo, BCu;
    v128 Da, De, Di, Do, Du;
    v128 Eba, Ebe, Ebi, Ebo, Ebu;
    v128 Ega, Ege, Egi, Ego, Egu;
    v128 Eka, Eke, Eki, Eko, Eku;
    v128 Ema, Eme, Emi, Emo, Emu;
    v128 Esa, Ese, Esi, Eso, Esu;
    v128 t1, t2, t3, t4, t5;

    //copyFromState(A, state)
    Aba = state[0];
    Abe = state[1];
    Abi = state[2];
    Abo = state[3];
    Abu = state[4];
    Aga = state[5];
    Age = state[6];
    Agi = state[7];
    Ago = state[8];
    Agu = state[9];
    Aka = state[10];
    Ake = state[11];
    Aki = state[12];
    Ako = state[13];
    Aku = state[14];
    Ama = state[15];
    Ame = state[16];
    Ami = state[17];
    Amo = state[18];
    Amu = state[19];
    Asa = state[20];
    Ase = state[21];
    Asi = state[22];
    Aso = state[23];
    Asu = state[24];

    for (int round = 0; round < NROUNDS; round += 2)
    {
        //    prepareTheta
        vXOR4(BCa, Aba, Aga, Aka, Ama, Asa);
        vXOR4(BCe, Abe, Age, Ake, Ame, Ase);
        vXOR4(BCi, Abi, Agi, Aki, Ami, Asi);
        vXOR4(BCo, Abo, Ago, Ako, Amo, Aso);
        vXOR4(BCu, Abu, Agu, Aku, Amu, Asu);

        //thetaRhoPiChiIotaPrepareTheta(round  , A, E)
        vrxor(Da, BCu, BCe);
        vrxor(De, BCa, BCi);
        vrxor(Di, BCe, BCo);
        vrxor(Do, BCi, BCu);
        vrxor(Du, BCo, BCa);

        vxor(BCa, Aba, Da);
        vxor(Age, Age, De);
        vxor(Aki, Aki, Di);
        vxor(Amo, Amo, Do);
        vxor(Asu, Asu, Du);
        vROL(BCe, Age, t1, 44);
        vROL(BCi, Aki, t2, 43);
        vROL(BCo, Amo, t3, 21);
        vROL(BCu, Asu, t4, 14);
        vXNA(Eba, BCa, BCe, BCi);
        t5 = vdupq_n_u64(KeccakF_RoundConstants[round]);
        vxor(Eba, Eba, t5);
        vXNA(Ebe, BCe, BCi, BCo);
        vXNA(Ebi, BCi, BCo, BCu);
        vXNA(Ebo, BCo, BCu, BCa);
        vXNA(Ebu, BCu, BCa, BCe);

        /////////
        vxor(Abo, Abo, Do);
        vxor(Agu, Agu, Du);
        vxor(Aka, Aka, Da);
        vxor(Ame, Ame, De);
        vxor(Asi, Asi, Di);
        vROL(BCa, Abo, t1, 28);
        vROL(BCe, Agu, t2, 20);
        vROL(BCi, Aka, t3, 3);
        vROL(BCo, Ame, t4, 45);
        vROL(BCu, Asi, t5, 61);
        vXNA(Ega, BCa, BCe, BCi);
        vXNA(Ege, BCe, BCi, BCo);
        vXNA(Egi, BCi, BCo, BCu);
        vXNA(Ego, BCo, BCu, BCa);
        vXNA(Egu, BCu, BCa, BCe);

        vxor(Abe, Abe, De);
        vxor(Agi, Agi, Di);
        vxor(Ako, Ako, Do);
        vxor(Amu, Amu, Du);
        vxor(Asa, Asa, Da);
        vROL(BCa, Abe, t1, 1);
        vROL(BCe, Agi, t2, 6);
        vROL(BCi, Ako, t3, 25);
        vROL(BCo, Amu, t4, 8);
        vROL(BCu, Asa, t5, 18);
        vXNA(Eka, BCa, BCe, BCi);
        vXNA(Eke, BCe, BCi, BCo);
        vXNA(Eki, BCi, BCo, BCu);
        vXNA(Eko, BCo, BCu, BCa);
        vXNA(Eku, BCu, BCa, BCe);

        vxor(Abu, Abu, Du);
        vxor(Aga, Aga, Da);
        vxor(Ake, Ake, De);
        vxor(Ami, Ami, Di);
        vxor(Aso, Aso, Do);
        vROL(BCa, Abu, t1, 27);
        vROL(BCe, Aga, t2, 36);
        vROL(BCi, Ake, t3, 10);
        vROL(BCo, Ami, t4, 15);
        vROL(BCu, Aso, t5, 56);
        vXNA(Ema, BCa, BCe, BCi);
        vXNA(Eme, BCe, BCi, BCo);
        vXNA(Emi, BCi, BCo, BCu);
        vXNA(Emo, BCo, BCu, BCa);
        vXNA(Emu, BCu, BCa, BCe);

        vxor(Abi, Abi, Di);
        vxor(Ago, Ago, Do);
        vxor(Aku, Aku, Du);
        vxor(Ama, Ama, Da);
        vxor(Ase, Ase, De);
        vROL(BCa, Abi, t1, 62);
        vROL(BCe, Ago, t2, 55);
        vROL(BCi, Aku, t3, 39);
        vROL(BCo, Ama, t4, 41);
        vROL(BCu, Ase, t5, 2);
        vXNA(Esa, BCa, BCe, BCi);
        vXNA(Ese, BCe, BCi, BCo);
        vXNA(Esi, BCi, BCo, BCu);
        vXNA(Eso, BCo, BCu, BCa);
        vXNA(Esu, BCu, BCa, BCe);

        //    prepareTheta
        vXOR4(BCa, Eba, Ega, Eka, Ema, Esa);
        vXOR4(BCe, Ebe, Ege, Eke, Eme, Ese);
        vXOR4(BCi, Ebi, Egi, Eki, Emi, Esi);
        vXOR4(BCo, Ebo, Ego, Eko, Emo, Eso);
        vXOR4(BCu, Ebu, Egu, Eku, Emu, Esu);

        //thetaRhoPiChiIotaPrepareTheta(round+1, E, A)
        vrxor(Da, BCu, BCe);
        vrxor(De, BCa, BCi);
        vrxor(Di, BCe, BCo);
        vrxor(Do, BCi, BCu);
        vrxor(Du, BCo, BCa);

        vxor(BCa, Eba, Da);
        vxor(Ege, Ege, De);
        vxor(Eki, Eki, Di);
        vxor(Emo, Emo, Do);
        vxor(Esu, Esu, Du);
        vROL(BCe, Ege, t1, 44);
        vROL(BCi, Eki, t2, 43);
        vROL(BCo, Emo, t3, 21);
        vROL(BCu, Esu, t4, 14);
        vXNA(Aba, BCa, BCe, BCi);
        t5 = vdupq_n_u64(KeccakF_RoundConstants[round + 1]);
        vxor(Aba, Aba, t5);
        vXNA(Abe, BCe, BCi, BCo);
        vXNA(Abi, BCi, BCo, BCu);
        vXNA(Abo, BCo, BCu, BCa);
        vXNA(Abu, BCu, BCa, BCe);

        vxor(Ebo, Ebo, Do);
        vxor(Egu, Egu, Du);
        vxor(Eka, Eka, Da);
        vxor(Eme, Eme, De);
        vxor(Esi, Esi, Di);
        vROL(BCa, Ebo, t1, 28);
        vROL(BCe, Egu, t2, 20);
        vROL(BCi, Eka, t3, 3);
        vROL(BCo, Eme, t4, 45);
        vROL(BCu, Esi, t5, 61);
        vXNA(Aga, BCa, BCe, BCi);
        vXNA(Age, BCe, BCi, BCo);
        vXNA(Agi, BCi, BCo, BCu);
        vXNA(Ago, BCo, BCu, BCa);
        vXNA(Agu, BCu, BCa, BCe);

        vxor(Ebe, Ebe, De);
        vxor(Egi, Egi, Di);
        vxor(Eko, Eko, Do);
        vxor(Emu, Emu, Du);
        vxor(Esa, Esa, Da);
        vROL(BCa, Ebe, t1, 1);
        vROL(BCe, Egi, t2, 6);
        vROL(BCi, Eko, t3, 25);
        vROL(BCo, Emu, t4, 8);
        vROL(BCu, Esa, t5, 18);
        vXNA(Aka, BCa, BCe, BCi);
        vXNA(Ake, BCe, BCi, BCo);
        vXNA(Aki, BCi, BCo, BCu);
        vXNA(Ako, BCo, BCu, BCa);
        vXNA(Aku, BCu, BCa, BCe);

        vxor(Ebu, Ebu, Du);
        vxor(Ega, Ega, Da);
        vxor(Eke, Eke, De);
        vxor(Emi, Emi, Di);
        vxor(Eso, Eso, Do);
        vROL(BCa, Ebu, t1, 27);
        vROL(BCe, Ega, t2, 36);
        vROL(BCi, Eke, t3, 10);
        vROL(BCo, Emi, t4, 15);
        vROL(BCu, Eso, t5, 56);
        vXNA(Ama, BCa, BCe, BCi);
        vXNA(Ame, BCe, BCi, BCo);
        vXNA(Ami, BCi, BCo, BCu);
        vXNA(Amo, BCo, BCu, BCa);
        vXNA(Amu, BCu, BCa, BCe);

        vxor(Ebi, Ebi, Di);
        vxor(Ego, Ego, Do);
        vxor(Eku, Eku, Du);
        vxor(Ema, Ema, Da);
        vxor(Ese, Ese, De);
        vROL(BCa, Ebi, t1, 62);
        vROL(BCe, Ego, t2, 55);
        vROL(BCi, Eku, t3, 39);
        vROL(BCo, Ema, t4, 41);
        vROL(BCu, Ese, t5, 2);
        vXNA(Asa, BCa, BCe, BCi);
        vXNA(Ase, BCe, BCi, BCo);
        vXNA(Asi, BCi, BCo, BCu);
        vXNA(Aso, BCo, BCu, BCa);
        vXNA(Asu, BCu, BCa, BCe);
    }

    state[0]  =  Aba;
    state[1]  =  Abe;
    state[2]  =  Abi;
    state[3]  =  Abo;
    state[4]  =  Abu;
    state[5]  =  Aga;
    state[6]  =  Age;
    state[7]  =  Agi;
    state[8]  =  Ago;
    state[9]  =  Agu;
    state[10] =  Aka;
    state[11] =  Ake;
    state[12] =  Aki;
    state[13] =  Ako;
    state[14] =  Aku;
    state[15] =  Ama;
    state[16] =  Ame;
    state[17] =  Ami;
    state[18] =  Amo;
    state[19] =  Amu;
    state[20] =  Asa;
    state[21] =  Ase;
    state[22] =  Asi;
    state[23] =  Aso;
    state[24] =  Asu;
}


int main()
{
    v128 state[25];
    v128 t,u;
    for (int i  =0; i < 25; i++)
    {
        t = vdupq_n_u64(rand());
        u = vdupq_n_u64(rand());
        vxor(state[i], t, u);
    }

    KeccakF1600_StatePermute(state);
    
}
