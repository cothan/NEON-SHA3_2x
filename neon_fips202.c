#include <arm_neon.h>
#include <stdio.h>
#include <stdlib.h>
#include <papi.h>
#include <time.h>

#define NROUNDS 24
#define ROL(a, offset) ((a << offset) ^ (a >> (64-offset)))

// Define NEON operation

// c = load(ptr)
#define vload(ptr) vld1q_u64(ptr);
// ptr <= c;
#define vstore(ptr, c) vst1q_u64(ptr, c);
// c = a ^ b
#define vxor(c, a, b) c = veorq_u64(a, b);
// Rotate by n bit ((a << offset) ^ (a >> (64-offset)))
#define vROL(out, a, offset) \
    out = vshlq_n_u64(a, offset);\
    out = vsriq_n_u64(out, a, 64 - offset);
// Xor chain: out = a ^ b ^ c ^ d ^ e
#define vXOR4(out, a, b, c, d, e) \
    out = veorq_u64(a, b);\
    out = veorq_u64(out, c);\
    out = veorq_u64(out, d);\
    out = veorq_u64(out, e);
    // out = veorq_u64(veorq_u64(a, b), veorq_u64(c, veorq_u64(d, e)));
// Not And c = ~a & b
// #define vbic(c, a, b) c = vbicq_u64(b, a);

// Xor Not And: out = a ^ ( (~b) & c)
#define vXNA(out, a, b, c) \
    out = vbicq_u64(c, b);\
    out = veorq_u64(out, a);

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


void print_state(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e)
{
    static int count = 0;
    // printf("%d : %lx -- %lx -- %lx -- %lx -- %lx\n", count++, a, b, c, d, e);
    // printf("%d : %lu -- %lu -- %lu -- %lu -- %lu\n", count++, a, b, c, d, e);
}


void KeccakF1600_StatePermute(uint64_t state[25])
{
        int round;

        uint64_t Aba, Abe, Abi, Abo, Abu;
        uint64_t Aga, Age, Agi, Ago, Agu;
        uint64_t Aka, Ake, Aki, Ako, Aku;
        uint64_t Ama, Ame, Ami, Amo, Amu;
        uint64_t Asa, Ase, Asi, Aso, Asu;
        uint64_t BCa, BCe, BCi, BCo, BCu;
        uint64_t Da, De, Di, Do, Du;
        uint64_t Eba, Ebe, Ebi, Ebo, Ebu;
        uint64_t Ega, Ege, Egi, Ego, Egu;
        uint64_t Eka, Eke, Eki, Eko, Eku;
        uint64_t Ema, Eme, Emi, Emo, Emu;
        uint64_t Esa, Ese, Esi, Eso, Esu;

        //copyFromState(A, state)
        Aba = state[ 0];
        Abe = state[ 1];
        Abi = state[ 2];
        Abo = state[ 3];
        Abu = state[ 4];
        Aga = state[ 5];
        Age = state[ 6];
        Agi = state[ 7];
        Ago = state[ 8];
        Agu = state[ 9];
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

        for( round = 0; round < NROUNDS; round += 2 )
        {
            //    prepareTheta
            BCa = Aba^Aga^Aka^Ama^Asa;
            BCe = Abe^Age^Ake^Ame^Ase;
            BCi = Abi^Agi^Aki^Ami^Asi;
            BCo = Abo^Ago^Ako^Amo^Aso;
            BCu = Abu^Agu^Aku^Amu^Asu;

            print_state(BCa, BCe, BCi, BCo, BCu);

            //thetaRhoPiChiIotaPrepareTheta(round  , A, E)
            Da = BCu^ROL(BCe, 1);
            De = BCa^ROL(BCi, 1);
            Di = BCe^ROL(BCo, 1);
            Do = BCi^ROL(BCu, 1);
            Du = BCo^ROL(BCa, 1);

            print_state(Da,De,Di,Do,Du);

            Aba ^= Da;
            BCa = Aba;
            Age ^= De;
            BCe = ROL(Age, 44);
            Aki ^= Di;
            BCi = ROL(Aki, 43);
            Amo ^= Do;
            BCo = ROL(Amo, 21);
            Asu ^= Du;
            BCu = ROL(Asu, 14);
            print_state(BCa, BCe, BCi, BCo, BCu);
            Eba =   BCa ^((~BCe)&  BCi );
            Eba ^= (uint64_t)KeccakF_RoundConstants[round];
            Ebe =   BCe ^((~BCi)&  BCo );
            Ebi =   BCi ^((~BCo)&  BCu );
            Ebo =   BCo ^((~BCu)&  BCa );
            Ebu =   BCu ^((~BCa)&  BCe );

            print_state(Eba, Ebe, Ebi, Ebo, Ebu);

            Abo ^= Do;
            BCa = ROL(Abo, 28);
            Agu ^= Du;
            BCe = ROL(Agu, 20);
            Aka ^= Da;
            BCi = ROL(Aka,  3);
            Ame ^= De;
            BCo = ROL(Ame, 45);
            Asi ^= Di;
            BCu = ROL(Asi, 61);
            Ega =   BCa ^((~BCe)&  BCi );
            Ege =   BCe ^((~BCi)&  BCo );
            Egi =   BCi ^((~BCo)&  BCu );
            Ego =   BCo ^((~BCu)&  BCa );
            Egu =   BCu ^((~BCa)&  BCe );

            print_state(Ega, Ege, Egi, Ego, Egu);

            Abe ^= De;
            BCa = ROL(Abe,  1);
            Agi ^= Di;
            BCe = ROL(Agi,  6);
            Ako ^= Do;
            BCi = ROL(Ako, 25);
            Amu ^= Du;
            BCo = ROL(Amu,  8);
            Asa ^= Da;
            BCu = ROL(Asa, 18);
            Eka =   BCa ^((~BCe)&  BCi );
            Eke =   BCe ^((~BCi)&  BCo );
            Eki =   BCi ^((~BCo)&  BCu );
            Eko =   BCo ^((~BCu)&  BCa );
            Eku =   BCu ^((~BCa)&  BCe );
            
            print_state(Eka, Eke, Eki, Eko, Eku);

            Abu ^= Du;
            BCa = ROL(Abu, 27);
            Aga ^= Da;
            BCe = ROL(Aga, 36);
            Ake ^= De;
            BCi = ROL(Ake, 10);
            Ami ^= Di;
            BCo = ROL(Ami, 15);
            Aso ^= Do;
            BCu = ROL(Aso, 56);
            Ema =   BCa ^((~BCe)&  BCi );
            Eme =   BCe ^((~BCi)&  BCo );
            Emi =   BCi ^((~BCo)&  BCu );
            Emo =   BCo ^((~BCu)&  BCa );
            Emu =   BCu ^((~BCa)&  BCe );

            print_state(Ema, Eme, Emi, Emo, Emu);

            Abi ^= Di;
            BCa = ROL(Abi, 62);
            Ago ^= Do;
            BCe = ROL(Ago, 55);
            Aku ^= Du;
            BCi = ROL(Aku, 39);
            Ama ^= Da;
            BCo = ROL(Ama, 41);
            Ase ^= De;
            BCu = ROL(Ase,  2);
            Esa =   BCa ^((~BCe)&  BCi );
            Ese =   BCe ^((~BCi)&  BCo );
            Esi =   BCi ^((~BCo)&  BCu );
            Eso =   BCo ^((~BCu)&  BCa );
            Esu =   BCu ^((~BCa)&  BCe );

            print_state(Esa, Ese, Esi, Eso, Esu);

            //    prepareTheta
            BCa = Eba^Ega^Eka^Ema^Esa;
            BCe = Ebe^Ege^Eke^Eme^Ese;
            BCi = Ebi^Egi^Eki^Emi^Esi;
            BCo = Ebo^Ego^Eko^Emo^Eso;
            BCu = Ebu^Egu^Eku^Emu^Esu;

            print_state(BCa, BCe, BCi, BCo, BCu);

            //thetaRhoPiChiIotaPrepareTheta(round+1, E, A)
            Da = BCu^ROL(BCe, 1);
            De = BCa^ROL(BCi, 1);
            Di = BCe^ROL(BCo, 1);
            Do = BCi^ROL(BCu, 1);
            Du = BCo^ROL(BCa, 1);

            print_state(Da, De, Di, Do, Du);

            Eba ^= Da;
            BCa = Eba;
            Ege ^= De;
            BCe = ROL(Ege, 44);
            Eki ^= Di;
            BCi = ROL(Eki, 43);
            Emo ^= Do;
            BCo = ROL(Emo, 21);
            Esu ^= Du;
            BCu = ROL(Esu, 14);
            Aba =   BCa ^((~BCe)&  BCi );
            Aba ^= (uint64_t)KeccakF_RoundConstants[round+1];
            Abe =   BCe ^((~BCi)&  BCo );
            Abi =   BCi ^((~BCo)&  BCu );
            Abo =   BCo ^((~BCu)&  BCa );
            Abu =   BCu ^((~BCa)&  BCe );
        
            print_state(Aba, Abe, Abi, Abo, Abu);

            Ebo ^= Do;
            BCa = ROL(Ebo, 28);
            Egu ^= Du;
            BCe = ROL(Egu, 20);
            Eka ^= Da;
            BCi = ROL(Eka, 3);
            Eme ^= De;
            BCo = ROL(Eme, 45);
            Esi ^= Di;
            BCu = ROL(Esi, 61);
            Aga =   BCa ^((~BCe)&  BCi );
            Age =   BCe ^((~BCi)&  BCo );
            Agi =   BCi ^((~BCo)&  BCu );
            Ago =   BCo ^((~BCu)&  BCa );
            Agu =   BCu ^((~BCa)&  BCe );

            print_state(Aga, Age, Agi, Ago, Agu);

            Ebe ^= De;
            BCa = ROL(Ebe, 1);
            Egi ^= Di;
            BCe = ROL(Egi, 6);
            Eko ^= Do;
            BCi = ROL(Eko, 25);
            Emu ^= Du;
            BCo = ROL(Emu, 8);
            Esa ^= Da;
            BCu = ROL(Esa, 18);
            Aka =   BCa ^((~BCe)&  BCi );
            Ake =   BCe ^((~BCi)&  BCo );
            Aki =   BCi ^((~BCo)&  BCu );
            Ako =   BCo ^((~BCu)&  BCa );
            Aku =   BCu ^((~BCa)&  BCe );
        
            print_state(Aka, Ake, Aki, Ako, Aku);

            Ebu ^= Du;
            BCa = ROL(Ebu, 27);
            Ega ^= Da;
            BCe = ROL(Ega, 36);
            Eke ^= De;
            BCi = ROL(Eke, 10);
            Emi ^= Di;
            BCo = ROL(Emi, 15);
            Eso ^= Do;
            BCu = ROL(Eso, 56);
            Ama =   BCa ^((~BCe)&  BCi );
            Ame =   BCe ^((~BCi)&  BCo );
            Ami =   BCi ^((~BCo)&  BCu );
            Amo =   BCo ^((~BCu)&  BCa );
            Amu =   BCu ^((~BCa)&  BCe );

            print_state(Ama, Ame, Ami, Amo, Amu);

            Ebi ^= Di;
            BCa = ROL(Ebi, 62);
            Ego ^= Do;
            BCe = ROL(Ego, 55);
            Eku ^= Du;
            BCi = ROL(Eku, 39);
            Ema ^= Da;
            BCo = ROL(Ema, 41);
            Ese ^= De;
            BCu = ROL(Ese, 2);
            Asa =   BCa ^((~BCe)&  BCi );
            Ase =   BCe ^((~BCi)&  BCo );
            Asi =   BCi ^((~BCo)&  BCu );
            Aso =   BCo ^((~BCu)&  BCa );
            Asu =   BCu ^((~BCa)&  BCe );
            
            print_state(Asa, Ase, Asi, Aso, Asu);
        }

        //copyToState(state, A)
        state[ 0] = Aba;
        state[ 1] = Abe;
        state[ 2] = Abi;
        state[ 3] = Abo;
        state[ 4] = Abu;
        state[ 5] = Aga;
        state[ 6] = Age;
        state[ 7] = Agi;
        state[ 8] = Ago;
        state[ 9] = Agu;
        state[10] = Aka;
        state[11] = Ake;
        state[12] = Aki;
        state[13] = Ako;
        state[14] = Aku;
        state[15] = Ama;
        state[16] = Ame;
        state[17] = Ami;
        state[18] = Amo;
        state[19] = Amu;
        state[20] = Asa;
        state[21] = Ase;
        state[22] = Asi;
        state[23] = Aso;
        state[24] = Asu;
}

typedef uint64x2_t v128;

static inline
void neon_KeccakF1600_StatePermute(v128 state[25])
{
    v128 Aba, Abe, Abi, Abo, Abu;
    v128 Aga, Age, Agi, Ago, Agu;
    v128 Aka, Ake, Aki, Ako, Aku;
    v128 Ama, Ame, Ami, Amo, Amu;
    v128 Asa, Ase, Asi, Aso, Asu;
    v128 BCa, BCe, BCi, BCo, BCu; // C
    v128 Da, De, Di, Do, Du; // D 
    v128 Eba, Ebe, Ebi, Ebo, Ebu;
    v128 Ega, Ege, Egi, Ego, Egu;
    v128 Eka, Eke, Eki, Eko, Eku;
    v128 Ema, Eme, Emi, Emo, Emu;
    v128 Esa, Ese, Esi, Eso, Esu;

    //copyFromState(A, state)
    Aba = state[ 0];
    Abe = state[ 1];
    Abi = state[ 2];
    Abo = state[ 3];
    Abu = state[ 4];
    Aga = state[ 5];
    Age = state[ 6];
    Agi = state[ 7];
    Ago = state[ 8];
    Agu = state[ 9];
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
        vROL(Da, BCe, 1);
        vROL(De, BCi, 1);
        vROL(Di, BCo, 1);
        vROL(Do, BCu, 1);
        vROL(Du, BCa, 1);
        vxor(Da, BCu, Da);
        vxor(De, BCa, De);
        vxor(Di, BCe, Di);
        vxor(Do, BCi, Do);
        vxor(Du, BCo, Du);

        vxor(Aba, Aba, Da);
        vxor(Age, Age, De);
        vxor(Aki, Aki, Di);
        vxor(Amo, Amo, Do);
        vxor(Asu, Asu, Du);
        vROL(BCe, Age, 44);
        vROL(BCi, Aki, 43);
        vROL(BCo, Amo, 21);
        vROL(BCu, Asu, 14);
        vXNA(Eba, Aba, BCe, BCi);
        vxor(Eba, Eba, vdupq_n_u64(KeccakF_RoundConstants[round]));
        vXNA(Ebe, BCe, BCi, BCo);
        vXNA(Ebi, BCi, BCo, BCu);
        vXNA(Ebo, BCo, BCu, Aba);
        vXNA(Ebu, BCu, Aba, BCe);

        vxor(Abo, Abo, Do);
        vxor(Agu, Agu, Du);
        vxor(Aka, Aka, Da);
        vxor(Ame, Ame, De);
        vxor(Asi, Asi, Di);
        vROL(BCa, Abo, 28);
        vROL(BCe, Agu, 20);
        vROL(BCi, Aka, 3);
        vROL(BCo, Ame, 45);
        vROL(BCu, Asi, 61);
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
        vROL(BCa, Abe, 1);
        vROL(BCe, Agi, 6);
        vROL(BCi, Ako, 25);
        vROL(BCo, Amu, 8);
        vROL(BCu, Asa, 18);
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
        vROL(BCa, Abu, 27);
        vROL(BCe, Aga, 36);
        vROL(BCi, Ake, 10);
        vROL(BCo, Ami, 15);
        vROL(BCu, Aso, 56);
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
        vROL(BCa, Abi, 62);
        vROL(BCe, Ago, 55);
        vROL(BCi, Aku, 39);
        vROL(BCo, Ama, 41);
        vROL(BCu, Ase, 2);
        vXNA(Esa, BCa, BCe, BCi);
        vXNA(Ese, BCe, BCi, BCo);
        vXNA(Esi, BCi, BCo, BCu);
        vXNA(Eso, BCo, BCu, BCa);
        vXNA(Esu, BCu, BCa, BCe);

        // Next Round

        //    prepareTheta
        vXOR4(BCa, Eba, Ega, Eka, Ema, Esa);
        vXOR4(BCe, Ebe, Ege, Eke, Eme, Ese);
        vXOR4(BCi, Ebi, Egi, Eki, Emi, Esi);
        vXOR4(BCo, Ebo, Ego, Eko, Emo, Eso);
        vXOR4(BCu, Ebu, Egu, Eku, Emu, Esu);

        //thetaRhoPiChiIotaPrepareTheta(round+1, E, A)
        vROL(Da, BCe, 1);
        vROL(De, BCi, 1);
        vROL(Di, BCo, 1);
        vROL(Do, BCu, 1);
        vROL(Du, BCa, 1);
        vxor(Da, BCu, Da);
        vxor(De, BCa, De);
        vxor(Di, BCe, Di);
        vxor(Do, BCi, Do);
        vxor(Du, BCo, Du);

        vxor(Eba, Eba, Da);
        vxor(Ege, Ege, De);
        vxor(Eki, Eki, Di);
        vxor(Emo, Emo, Do);
        vxor(Esu, Esu, Du);
        vROL(BCe, Ege, 44);
        vROL(BCi, Eki, 43);
        vROL(BCo, Emo, 21);
        vROL(BCu, Esu, 14);
        vXNA(Aba, Eba, BCe, BCi);
        vxor(Aba, Aba, vdupq_n_u64(KeccakF_RoundConstants[round + 1]));
        vXNA(Abe, BCe, BCi, BCo);
        vXNA(Abi, BCi, BCo, BCu);
        vXNA(Abo, BCo, BCu, Eba);
        vXNA(Abu, BCu, Eba, BCe);

        vxor(Ebo, Ebo, Do);
        vxor(Egu, Egu, Du);
        vxor(Eka, Eka, Da);
        vxor(Eme, Eme, De);
        vxor(Esi, Esi, Di);
        vROL(BCa, Ebo, 28);
        vROL(BCe, Egu, 20);
        vROL(BCi, Eka, 3);
        vROL(BCo, Eme, 45);
        vROL(BCu, Esi, 61);
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
        vROL(BCa, Ebe, 1);
        vROL(BCe, Egi, 6);
        vROL(BCi, Eko, 25);
        vROL(BCo, Emu, 8);
        vROL(BCu, Esa, 18);
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
        vROL(BCa, Ebu, 27);
        vROL(BCe, Ega, 36);
        vROL(BCi, Eke, 10);
        vROL(BCo, Emi, 15);
        vROL(BCu, Eso, 56);
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
        vROL(BCa, Ebi, 62);
        vROL(BCe, Ego, 55);
        vROL(BCi, Eku, 39);
        vROL(BCo, Ema, 41);
        vROL(BCu, Ese, 2);
        vXNA(Asa, BCa, BCe, BCi);
        vXNA(Ase, BCe, BCi, BCo);
        vXNA(Asi, BCi, BCo, BCu);
        vXNA(Aso, BCo, BCu, BCa);
        vXNA(Asu, BCu, BCa, BCe);
    }

    state[ 0] = Aba;
    state[ 1] = Abe;
    state[ 2] = Abi;
    state[ 3] = Abo;
    state[ 4] = Abu;
    state[ 5] = Aga;
    state[ 6] = Age;
    state[ 7] = Agi;
    state[ 8] = Ago;
    state[ 9] = Agu;
    state[10] = Aka;
    state[11] = Ake;
    state[12] = Aki;
    state[13] = Ako;
    state[14] = Aku;
    state[15] = Ama;
    state[16] = Ame;
    state[17] = Ami;
    state[18] = Amo;
    state[19] = Amu;
    state[20] = Asa;
    state[21] = Ase;
    state[22] = Asi;
    state[23] = Aso;
    state[24] = Asu;
}

#define TESTS 10000

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
    printf("FIPS202.c: %lld\n", end - start);
        

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
