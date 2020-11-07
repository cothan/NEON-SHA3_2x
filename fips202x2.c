#include <arm_neon.h>
#include <stddef.h>
#include "fips202x2.h"

#define NROUNDS 24

// Define NEON operation
// c = load(ptr)
#define vload(ptr) vld1q_u64(ptr);
// ptr <= c;
#define vstore(ptr, c) vst1q_u64(ptr, c);
// c = a ^ b
#define vxor(c, a, b) c = veorq_u64(a, b);
// Rotate by n bit ((a << offset) ^ (a >> (64-offset)))
#define vROL(out, a, offset)    \
  out = vshlq_n_u64(a, offset); \
  out = vsriq_n_u64(out, a, 64 - offset);
// Xor chain: out = a ^ b ^ c ^ d ^ e
#define vXOR4(out, a, b, c, d, e) \
  out = veorq_u64(a, b);          \
  out = veorq_u64(out, c);        \
  out = veorq_u64(out, d);        \
  out = veorq_u64(out, e);
// Not And c = ~a & b
// #define vbic(c, a, b) c = vbicq_u64(b, a);
// Xor Not And: out = a ^ ( (~b) & c)
#define vXNA(out, a, b, c) \
  out = vbicq_u64(c, b);   \
  out = veorq_u64(out, a);
// Rotate by 1 bit, then XOR: a ^ ROL(b): SHA1 instruction, not support
#define vrxor(c, a, b) c = vrax1q_u64(a, b);
// End Define

/* Keccak round constants */
static const uint64_t neon_KeccakF_RoundConstants[NROUNDS] = {
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

/* ============8888888888888888888888=============*/

#define KeccakF1600_StatePermutex2_macro(state, Estate, Bstate, Dstate)            \
  for (int round = 0; round < NROUNDS; round += 2)                                 \
  {                                                                                \
    vXOR4(Bstate[0], state[0], state[5], state[10], state[15], state[20]);         \
    vXOR4(Bstate[1], state[1], state[6], state[11], state[16], state[21]);         \
    vXOR4(Bstate[2], state[2], state[7], state[12], state[17], state[22]);         \
    vXOR4(Bstate[3], state[3], state[8], state[13], state[18], state[23]);         \
    vXOR4(Bstate[4], state[4], state[9], state[14], state[19], state[24]);         \
    vROL(Dstate[0], Bstate[1], 1);                                                 \
    vxor(Dstate[0], Bstate[4], Dstate[0]);                                         \
    vROL(Dstate[1], Bstate[2], 1);                                                 \
    vxor(Dstate[1], Bstate[0], Dstate[1]);                                         \
    vROL(Dstate[2], Bstate[3], 1);                                                 \
    vxor(Dstate[2], Bstate[1], Dstate[2]);                                         \
    vROL(Dstate[3], Bstate[4], 1);                                                 \
    vxor(Dstate[3], Bstate[2], Dstate[3]);                                         \
    vROL(Dstate[4], Bstate[0], 1);                                                 \
    vxor(Dstate[4], Bstate[3], Dstate[4]);                                         \
    vxor(state[0], state[0], Dstate[0]);                                           \
    vxor(state[6], state[6], Dstate[1]);                                           \
    vROL(Bstate[1], state[6], 44);                                                 \
    vxor(state[12], state[12], Dstate[2]);                                         \
    vROL(Bstate[2], state[12], 43);                                                \
    vxor(state[18], state[18], Dstate[3]);                                         \
    vROL(Bstate[3], state[18], 21);                                                \
    vxor(state[24], state[24], Dstate[4]);                                         \
    vROL(Bstate[4], state[24], 14);                                                \
    vXNA(Estate[0], state[0], Bstate[1], Bstate[2]);                               \
    vxor(Estate[0], Estate[0], vdupq_n_u64(neon_KeccakF_RoundConstants[round]));   \
    vXNA(Estate[1], Bstate[1], Bstate[2], Bstate[3]);                              \
    vXNA(Estate[2], Bstate[2], Bstate[3], Bstate[4]);                              \
    vXNA(Estate[3], Bstate[3], Bstate[4], state[0]);                               \
    vXNA(Estate[4], Bstate[4], state[0], Bstate[1]);                               \
    vxor(state[3], state[3], Dstate[3]);                                           \
    vROL(Bstate[0], state[3], 28);                                                 \
    vxor(state[9], state[9], Dstate[4]);                                           \
    vROL(Bstate[1], state[9], 20);                                                 \
    vxor(state[10], state[10], Dstate[0]);                                         \
    vROL(Bstate[2], state[10], 3);                                                 \
    vxor(state[16], state[16], Dstate[1]);                                         \
    vROL(Bstate[3], state[16], 45);                                                \
    vxor(state[22], state[22], Dstate[2]);                                         \
    vROL(Bstate[4], state[22], 61);                                                \
    vXNA(Estate[5], Bstate[0], Bstate[1], Bstate[2]);                              \
    vXNA(Estate[6], Bstate[1], Bstate[2], Bstate[3]);                              \
    vXNA(Estate[7], Bstate[2], Bstate[3], Bstate[4]);                              \
    vXNA(Estate[8], Bstate[3], Bstate[4], Bstate[0]);                              \
    vXNA(Estate[9], Bstate[4], Bstate[0], Bstate[1]);                              \
    vxor(state[1], state[1], Dstate[1]);                                           \
    vROL(Bstate[0], state[1], 1);                                                  \
    vxor(state[7], state[7], Dstate[2]);                                           \
    vROL(Bstate[1], state[7], 6);                                                  \
    vxor(state[13], state[13], Dstate[3]);                                         \
    vROL(Bstate[2], state[13], 25);                                                \
    vxor(state[19], state[19], Dstate[4]);                                         \
    vROL(Bstate[3], state[19], 8);                                                 \
    vxor(state[20], state[20], Dstate[0]);                                         \
    vROL(Bstate[4], state[20], 18);                                                \
    vXNA(Estate[10], Bstate[0], Bstate[1], Bstate[2]);                             \
    vXNA(Estate[11], Bstate[1], Bstate[2], Bstate[3]);                             \
    vXNA(Estate[12], Bstate[2], Bstate[3], Bstate[4]);                             \
    vXNA(Estate[13], Bstate[3], Bstate[4], Bstate[0]);                             \
    vXNA(Estate[14], Bstate[4], Bstate[0], Bstate[1]);                             \
    vxor(state[4], state[4], Dstate[4]);                                           \
    vROL(Bstate[0], state[4], 27);                                                 \
    vxor(state[5], state[5], Dstate[0]);                                           \
    vROL(Bstate[1], state[5], 36);                                                 \
    vxor(state[11], state[11], Dstate[1]);                                         \
    vROL(Bstate[2], state[11], 10);                                                \
    vxor(state[17], state[17], Dstate[2]);                                         \
    vROL(Bstate[3], state[17], 15);                                                \
    vxor(state[23], state[23], Dstate[3]);                                         \
    vROL(Bstate[4], state[23], 56);                                                \
    vXNA(Estate[15], Bstate[0], Bstate[1], Bstate[2]);                             \
    vXNA(Estate[16], Bstate[1], Bstate[2], Bstate[3]);                             \
    vXNA(Estate[17], Bstate[2], Bstate[3], Bstate[4]);                             \
    vXNA(Estate[18], Bstate[3], Bstate[4], Bstate[0]);                             \
    vXNA(Estate[19], Bstate[4], Bstate[0], Bstate[1]);                             \
    vxor(state[2], state[2], Dstate[2]);                                           \
    vROL(Bstate[0], state[2], 62);                                                 \
    vxor(state[8], state[8], Dstate[3]);                                           \
    vROL(Bstate[1], state[8], 55);                                                 \
    vxor(state[14], state[14], Dstate[4]);                                         \
    vROL(Bstate[2], state[14], 39);                                                \
    vxor(state[15], state[15], Dstate[0]);                                         \
    vROL(Bstate[3], state[15], 41);                                                \
    vxor(state[21], state[21], Dstate[1]);                                         \
    vROL(Bstate[4], state[21], 2);                                                 \
    vXNA(Estate[20], Bstate[0], Bstate[1], Bstate[2]);                             \
    vXNA(Estate[21], Bstate[1], Bstate[2], Bstate[3]);                             \
    vXNA(Estate[22], Bstate[2], Bstate[3], Bstate[4]);                             \
    vXNA(Estate[23], Bstate[3], Bstate[4], Bstate[0]);                             \
    vXNA(Estate[24], Bstate[4], Bstate[0], Bstate[1]);                             \
    vXOR4(Bstate[0], Estate[0], Estate[5], Estate[10], Estate[15], Estate[20]);    \
    vXOR4(Bstate[1], Estate[1], Estate[6], Estate[11], Estate[16], Estate[21]);    \
    vXOR4(Bstate[2], Estate[2], Estate[7], Estate[12], Estate[17], Estate[22]);    \
    vXOR4(Bstate[3], Estate[3], Estate[8], Estate[13], Estate[18], Estate[23]);    \
    vXOR4(Bstate[4], Estate[4], Estate[9], Estate[14], Estate[19], Estate[24]);    \
    vROL(Dstate[0], Bstate[1], 1);                                                 \
    vxor(Dstate[0], Bstate[4], Dstate[0]);                                         \
    vROL(Dstate[1], Bstate[2], 1);                                                 \
    vxor(Dstate[1], Bstate[0], Dstate[1]);                                         \
    vROL(Dstate[2], Bstate[3], 1);                                                 \
    vxor(Dstate[2], Bstate[1], Dstate[2]);                                         \
    vROL(Dstate[3], Bstate[4], 1);                                                 \
    vxor(Dstate[3], Bstate[2], Dstate[3]);                                         \
    vROL(Dstate[4], Bstate[0], 1);                                                 \
    vxor(Dstate[4], Bstate[3], Dstate[4]);                                         \
    vxor(Estate[0], Estate[0], Dstate[0]);                                         \
    vxor(Estate[6], Estate[6], Dstate[1]);                                         \
    vROL(Bstate[1], Estate[6], 44);                                                \
    vxor(Estate[12], Estate[12], Dstate[2]);                                       \
    vROL(Bstate[2], Estate[12], 43);                                               \
    vxor(Estate[18], Estate[18], Dstate[3]);                                       \
    vROL(Bstate[3], Estate[18], 21);                                               \
    vxor(Estate[24], Estate[24], Dstate[4]);                                       \
    vROL(Bstate[4], Estate[24], 14);                                               \
    vXNA(state[0], Estate[0], Bstate[1], Bstate[2]);                               \
    vxor(state[0], state[0], vdupq_n_u64(neon_KeccakF_RoundConstants[round + 1])); \
    vXNA(state[1], Bstate[1], Bstate[2], Bstate[3]);                               \
    vXNA(state[2], Bstate[2], Bstate[3], Bstate[4]);                               \
    vXNA(state[3], Bstate[3], Bstate[4], Estate[0]);                               \
    vXNA(state[4], Bstate[4], Estate[0], Bstate[1]);                               \
    vxor(Estate[3], Estate[3], Dstate[3]);                                         \
    vROL(Bstate[0], Estate[3], 28);                                                \
    vxor(Estate[9], Estate[9], Dstate[4]);                                         \
    vROL(Bstate[1], Estate[9], 20);                                                \
    vxor(Estate[10], Estate[10], Dstate[0]);                                       \
    vROL(Bstate[2], Estate[10], 3);                                                \
    vxor(Estate[16], Estate[16], Dstate[1]);                                       \
    vROL(Bstate[3], Estate[16], 45);                                               \
    vxor(Estate[22], Estate[22], Dstate[2]);                                       \
    vROL(Bstate[4], Estate[22], 61);                                               \
    vXNA(state[5], Bstate[0], Bstate[1], Bstate[2]);                               \
    vXNA(state[6], Bstate[1], Bstate[2], Bstate[3]);                               \
    vXNA(state[7], Bstate[2], Bstate[3], Bstate[4]);                               \
    vXNA(state[8], Bstate[3], Bstate[4], Bstate[0]);                               \
    vXNA(state[9], Bstate[4], Bstate[0], Bstate[1]);                               \
    vxor(Estate[1], Estate[1], Dstate[1]);                                         \
    vROL(Bstate[0], Estate[1], 1);                                                 \
    vxor(Estate[7], Estate[7], Dstate[2]);                                         \
    vROL(Bstate[1], Estate[7], 6);                                                 \
    vxor(Estate[13], Estate[13], Dstate[3]);                                       \
    vROL(Bstate[2], Estate[13], 25);                                               \
    vxor(Estate[19], Estate[19], Dstate[4]);                                       \
    vROL(Bstate[3], Estate[19], 8);                                                \
    vxor(Estate[20], Estate[20], Dstate[0]);                                       \
    vROL(Bstate[4], Estate[20], 18);                                               \
    vXNA(state[10], Bstate[0], Bstate[1], Bstate[2]);                              \
    vXNA(state[11], Bstate[1], Bstate[2], Bstate[3]);                              \
    vXNA(state[12], Bstate[2], Bstate[3], Bstate[4]);                              \
    vXNA(state[13], Bstate[3], Bstate[4], Bstate[0]);                              \
    vXNA(state[14], Bstate[4], Bstate[0], Bstate[1]);                              \
    vxor(Estate[4], Estate[4], Dstate[4]);                                         \
    vROL(Bstate[0], Estate[4], 27);                                                \
    vxor(Estate[5], Estate[5], Dstate[0]);                                         \
    vROL(Bstate[1], Estate[5], 36);                                                \
    vxor(Estate[11], Estate[11], Dstate[1]);                                       \
    vROL(Bstate[2], Estate[11], 10);                                               \
    vxor(Estate[17], Estate[17], Dstate[2]);                                       \
    vROL(Bstate[3], Estate[17], 15);                                               \
    vxor(Estate[23], Estate[23], Dstate[3]);                                       \
    vROL(Bstate[4], Estate[23], 56);                                               \
    vXNA(state[15], Bstate[0], Bstate[1], Bstate[2]);                              \
    vXNA(state[16], Bstate[1], Bstate[2], Bstate[3]);                              \
    vXNA(state[17], Bstate[2], Bstate[3], Bstate[4]);                              \
    vXNA(state[18], Bstate[3], Bstate[4], Bstate[0]);                              \
    vXNA(state[19], Bstate[4], Bstate[0], Bstate[1]);                              \
    vxor(Estate[2], Estate[2], Dstate[2]);                                         \
    vROL(Bstate[0], Estate[2], 62);                                                \
    vxor(Estate[8], Estate[8], Dstate[3]);                                         \
    vROL(Bstate[1], Estate[8], 55);                                                \
    vxor(Estate[14], Estate[14], Dstate[4]);                                       \
    vROL(Bstate[2], Estate[14], 39);                                               \
    vxor(Estate[15], Estate[15], Dstate[0]);                                       \
    vROL(Bstate[3], Estate[15], 41);                                               \
    vxor(Estate[21], Estate[21], Dstate[1]);                                       \
    vROL(Bstate[4], Estate[21], 2);                                                \
    vXNA(state[20], Bstate[0], Bstate[1], Bstate[2]);                              \
    vXNA(state[21], Bstate[1], Bstate[2], Bstate[3]);                              \
    vXNA(state[22], Bstate[2], Bstate[3], Bstate[4]);                              \
    vXNA(state[23], Bstate[3], Bstate[4], Bstate[0]);                              \
    vXNA(state[24], Bstate[4], Bstate[0], Bstate[1]);                              \
  }

/*************************************************
* Name:        KeccakF1600_StatePermutex2
*
* Description: The Keccak F1600 Permutation
*
* Arguments:   - uint64_t *state: pointer to input/output Keccak state
**************************************************/
static inline __attribute__((always_inline))
void KeccakF1600_StatePermutex2(v128 state[25])
{
  v128 Aba, Abe, Abi, Abo, Abu;
  v128 Aga, Age, Agi, Ago, Agu;
  v128 Aka, Ake, Aki, Ako, Aku;
  v128 Ama, Ame, Ami, Amo, Amu;
  v128 Asa, Ase, Asi, Aso, Asu;
  v128 BCa, BCe, BCi, BCo, BCu; // tmp
  v128 Da, De, Di, Do, Du;      // D
  v128 Eba, Ebe, Ebi, Ebo, Ebu;
  v128 Ega, Ege, Egi, Ego, Egu;
  v128 Eka, Eke, Eki, Eko, Eku;
  v128 Ema, Eme, Emi, Emo, Emu;
  v128 Esa, Ese, Esi, Eso, Esu;

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
    vROL(Da, BCe, 1);
    vxor(Da, BCu, Da);
    vROL(De, BCi, 1);
    vxor(De, BCa, De);
    vROL(Di, BCo, 1);
    vxor(Di, BCe, Di);
    vROL(Do, BCu, 1);
    vxor(Do, BCi, Do);
    vROL(Du, BCa, 1);
    vxor(Du, BCo, Du);

    vxor(Aba, Aba, Da);
    vxor(Age, Age, De);
    vROL(BCe, Age, 44);
    vxor(Aki, Aki, Di);
    vROL(BCi, Aki, 43);
    vxor(Amo, Amo, Do);
    vROL(BCo, Amo, 21);
    vxor(Asu, Asu, Du);
    vROL(BCu, Asu, 14);
    vXNA(Eba, Aba, BCe, BCi);
    vxor(Eba, Eba, vdupq_n_u64(neon_KeccakF_RoundConstants[round]));
    vXNA(Ebe, BCe, BCi, BCo);
    vXNA(Ebi, BCi, BCo, BCu);
    vXNA(Ebo, BCo, BCu, Aba);
    vXNA(Ebu, BCu, Aba, BCe);

    vxor(Abo, Abo, Do);
    vROL(BCa, Abo, 28);
    vxor(Agu, Agu, Du);
    vROL(BCe, Agu, 20);
    vxor(Aka, Aka, Da);
    vROL(BCi, Aka, 3);
    vxor(Ame, Ame, De);
    vROL(BCo, Ame, 45);
    vxor(Asi, Asi, Di);
    vROL(BCu, Asi, 61);
    vXNA(Ega, BCa, BCe, BCi);
    vXNA(Ege, BCe, BCi, BCo);
    vXNA(Egi, BCi, BCo, BCu);
    vXNA(Ego, BCo, BCu, BCa);
    vXNA(Egu, BCu, BCa, BCe);

    vxor(Abe, Abe, De);
    vROL(BCa, Abe, 1);
    vxor(Agi, Agi, Di);
    vROL(BCe, Agi, 6);
    vxor(Ako, Ako, Do);
    vROL(BCi, Ako, 25);
    vxor(Amu, Amu, Du);
    vROL(BCo, Amu, 8);
    vxor(Asa, Asa, Da);
    vROL(BCu, Asa, 18);
    vXNA(Eka, BCa, BCe, BCi);
    vXNA(Eke, BCe, BCi, BCo);
    vXNA(Eki, BCi, BCo, BCu);
    vXNA(Eko, BCo, BCu, BCa);
    vXNA(Eku, BCu, BCa, BCe);

    vxor(Abu, Abu, Du);
    vROL(BCa, Abu, 27);
    vxor(Aga, Aga, Da);
    vROL(BCe, Aga, 36);
    vxor(Ake, Ake, De);
    vROL(BCi, Ake, 10);
    vxor(Ami, Ami, Di);
    vROL(BCo, Ami, 15);
    vxor(Aso, Aso, Do);
    vROL(BCu, Aso, 56);
    vXNA(Ema, BCa, BCe, BCi);
    vXNA(Eme, BCe, BCi, BCo);
    vXNA(Emi, BCi, BCo, BCu);
    vXNA(Emo, BCo, BCu, BCa);
    vXNA(Emu, BCu, BCa, BCe);

    vxor(Abi, Abi, Di);
    vROL(BCa, Abi, 62);
    vxor(Ago, Ago, Do);
    vROL(BCe, Ago, 55);
    vxor(Aku, Aku, Du);
    vROL(BCi, Aku, 39);
    vxor(Ama, Ama, Da);
    vROL(BCo, Ama, 41);
    vxor(Ase, Ase, De);
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
    vxor(Da, BCu, Da);
    vROL(De, BCi, 1);
    vxor(De, BCa, De);
    vROL(Di, BCo, 1);
    vxor(Di, BCe, Di);
    vROL(Do, BCu, 1);
    vxor(Do, BCi, Do);
    vROL(Du, BCa, 1);
    vxor(Du, BCo, Du);

    vxor(Eba, Eba, Da);
    vxor(Ege, Ege, De);
    vROL(BCe, Ege, 44);
    vxor(Eki, Eki, Di);
    vROL(BCi, Eki, 43);
    vxor(Emo, Emo, Do);
    vROL(BCo, Emo, 21);
    vxor(Esu, Esu, Du);
    vROL(BCu, Esu, 14);
    vXNA(Aba, Eba, BCe, BCi);
    vxor(Aba, Aba, vdupq_n_u64(neon_KeccakF_RoundConstants[round + 1]));
    vXNA(Abe, BCe, BCi, BCo);
    vXNA(Abi, BCi, BCo, BCu);
    vXNA(Abo, BCo, BCu, Eba);
    vXNA(Abu, BCu, Eba, BCe);

    vxor(Ebo, Ebo, Do);
    vROL(BCa, Ebo, 28);
    vxor(Egu, Egu, Du);
    vROL(BCe, Egu, 20);
    vxor(Eka, Eka, Da);
    vROL(BCi, Eka, 3);
    vxor(Eme, Eme, De);
    vROL(BCo, Eme, 45);
    vxor(Esi, Esi, Di);
    vROL(BCu, Esi, 61);
    vXNA(Aga, BCa, BCe, BCi);
    vXNA(Age, BCe, BCi, BCo);
    vXNA(Agi, BCi, BCo, BCu);
    vXNA(Ago, BCo, BCu, BCa);
    vXNA(Agu, BCu, BCa, BCe);

    vxor(Ebe, Ebe, De);
    vROL(BCa, Ebe, 1);
    vxor(Egi, Egi, Di);
    vROL(BCe, Egi, 6);
    vxor(Eko, Eko, Do);
    vROL(BCi, Eko, 25);
    vxor(Emu, Emu, Du);
    vROL(BCo, Emu, 8);
    vxor(Esa, Esa, Da);
    vROL(BCu, Esa, 18);
    vXNA(Aka, BCa, BCe, BCi);
    vXNA(Ake, BCe, BCi, BCo);
    vXNA(Aki, BCi, BCo, BCu);
    vXNA(Ako, BCo, BCu, BCa);
    vXNA(Aku, BCu, BCa, BCe);

    vxor(Ebu, Ebu, Du);
    vROL(BCa, Ebu, 27);
    vxor(Ega, Ega, Da);
    vROL(BCe, Ega, 36);
    vxor(Eke, Eke, De);
    vROL(BCi, Eke, 10);
    vxor(Emi, Emi, Di);
    vROL(BCo, Emi, 15);
    vxor(Eso, Eso, Do);
    vROL(BCu, Eso, 56);
    vXNA(Ama, BCa, BCe, BCi);
    vXNA(Ame, BCe, BCi, BCo);
    vXNA(Ami, BCi, BCo, BCu);
    vXNA(Amo, BCo, BCu, BCa);
    vXNA(Amu, BCu, BCa, BCe);

    vxor(Ebi, Ebi, Di);
    vROL(BCa, Ebi, 62);
    vxor(Ego, Ego, Do);
    vROL(BCe, Ego, 55);
    vxor(Eku, Eku, Du);
    vROL(BCi, Eku, 39);
    vxor(Ema, Ema, Da);
    vROL(BCo, Ema, 41);
    vxor(Ese, Ese, De);
    vROL(BCu, Ese, 2);
    vXNA(Asa, BCa, BCe, BCi);
    vXNA(Ase, BCe, BCi, BCo);
    vXNA(Asi, BCi, BCo, BCu);
    vXNA(Aso, BCo, BCu, BCa);
    vXNA(Asu, BCu, BCa, BCe);
  }

  state[0] = Aba;
  state[1] = Abe;
  state[2] = Abi;
  state[3] = Abo;
  state[4] = Abu;
  state[5] = Aga;
  state[6] = Age;
  state[7] = Agi;
  state[8] = Ago;
  state[9] = Agu;
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


static inline __attribute__((always_inline))
void KeccakF1600_StatePermutex2_rename(v128 state[25])
{
  v128 BCa, BCe, BCi, BCo, BCu; // tmp
  v128 Da, De, Di, Do, Du;      // D
  v128 Eba, Ebe, Ebi, Ebo, Ebu;
  v128 Ega, Ege, Egi, Ego, Egu;
  v128 Eka, Eke, Eki, Eko, Eku;
  v128 Ema, Eme, Emi, Emo, Emu;
  v128 Esa, Ese, Esi, Eso, Esu;

  for (int round = 0; round < NROUNDS; round += 2)
  {
    //    prepareTheta
    vXOR4(BCa, state[0], state[5], state[10], state[15], state[20]);
    vXOR4(BCe, state[1], state[6], state[11], state[16], state[21]);
    vXOR4(BCi, state[2], state[7], state[12], state[17], state[22]);
    vXOR4(BCo, state[3], state[8], state[13], state[18], state[23]);
    vXOR4(BCu, state[4], state[9], state[14], state[19], state[24]);

    //thetaRhoPiChiIotaPrepareTheta(round  , A, E)
    vROL(Da, BCe, 1);
    vxor(Da, BCu, Da);
    vROL(De, BCi, 1);
    vxor(De, BCa, De);
    vROL(Di, BCo, 1);
    vxor(Di, BCe, Di);
    vROL(Do, BCu, 1);
    vxor(Do, BCi, Do);
    vROL(Du, BCa, 1);
    vxor(Du, BCo, Du);

    vxor(state[0], state[0], Da);
    vxor(state[6], state[6], De);
    vROL(BCe, state[6], 44);
    vxor(state[12], state[12], Di);
    vROL(BCi, state[12], 43);
    vxor(state[18], state[18], Do);
    vROL(BCo, state[18], 21);
    vxor(state[24], state[24], Du);
    vROL(BCu, state[24], 14);
    vXNA(Eba, state[0], BCe, BCi);
    vxor(Eba, Eba, vdupq_n_u64(neon_KeccakF_RoundConstants[round]));
    vXNA(Ebe, BCe, BCi, BCo);
    vXNA(Ebi, BCi, BCo, BCu);
    vXNA(Ebo, BCo, BCu, state[0]);
    vXNA(Ebu, BCu, state[0], BCe);

    vxor(state[3], state[3], Do);
    vROL(BCa, state[3], 28);
    vxor(state[9], state[9], Du);
    vROL(BCe, state[9], 20);
    vxor(state[10], state[10], Da);
    vROL(BCi, state[10], 3);
    vxor(state[16], state[16], De);
    vROL(BCo, state[16], 45);
    vxor(state[22], state[22], Di);
    vROL(BCu, state[22], 61);
    vXNA(Ega, BCa, BCe, BCi);
    vXNA(Ege, BCe, BCi, BCo);
    vXNA(Egi, BCi, BCo, BCu);
    vXNA(Ego, BCo, BCu, BCa);
    vXNA(Egu, BCu, BCa, BCe);

    vxor(state[1], state[1], De);
    vROL(BCa, state[1], 1);
    vxor(state[7], state[7], Di);
    vROL(BCe, state[7], 6);
    vxor(state[13], state[13], Do);
    vROL(BCi, state[13], 25);
    vxor(state[19], state[19], Du);
    vROL(BCo, state[19], 8);
    vxor(state[20], state[20], Da);
    vROL(BCu, state[20], 18);
    vXNA(Eka, BCa, BCe, BCi);
    vXNA(Eke, BCe, BCi, BCo);
    vXNA(Eki, BCi, BCo, BCu);
    vXNA(Eko, BCo, BCu, BCa);
    vXNA(Eku, BCu, BCa, BCe);

    vxor(state[4], state[4], Du);
    vROL(BCa, state[4], 27);
    vxor(state[5], state[5], Da);
    vROL(BCe, state[5], 36);
    vxor(state[11], state[11], De);
    vROL(BCi, state[11], 10);
    vxor(state[17], state[17], Di);
    vROL(BCo, state[17], 15);
    vxor(state[23], state[23], Do);
    vROL(BCu, state[23], 56);
    vXNA(Ema, BCa, BCe, BCi);
    vXNA(Eme, BCe, BCi, BCo);
    vXNA(Emi, BCi, BCo, BCu);
    vXNA(Emo, BCo, BCu, BCa);
    vXNA(Emu, BCu, BCa, BCe);

    vxor(state[2], state[2], Di);
    vROL(BCa, state[2], 62);
    vxor(state[8], state[8], Do);
    vROL(BCe, state[8], 55);
    vxor(state[14], state[14], Du);
    vROL(BCi, state[14], 39);
    vxor(state[15], state[15], Da);
    vROL(BCo, state[15], 41);
    vxor(state[21], state[21], De);
    vROL(BCu, state[21], 2);
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
    vxor(Da, BCu, Da);
    vROL(De, BCi, 1);
    vxor(De, BCa, De);
    vROL(Di, BCo, 1);
    vxor(Di, BCe, Di);
    vROL(Do, BCu, 1);
    vxor(Do, BCi, Do);
    vROL(Du, BCa, 1);
    vxor(Du, BCo, Du);

    vxor(Eba, Eba, Da);
    vxor(Ege, Ege, De);
    vROL(BCe, Ege, 44);
    vxor(Eki, Eki, Di);
    vROL(BCi, Eki, 43);
    vxor(Emo, Emo, Do);
    vROL(BCo, Emo, 21);
    vxor(Esu, Esu, Du);
    vROL(BCu, Esu, 14);
    vXNA(state[0], Eba, BCe, BCi);
    vxor(state[0], state[0], vdupq_n_u64(neon_KeccakF_RoundConstants[round + 1]));
    vXNA(state[1], BCe, BCi, BCo);
    vXNA(state[2], BCi, BCo, BCu);
    vXNA(state[3], BCo, BCu, Eba);
    vXNA(state[4], BCu, Eba, BCe);

    vxor(Ebo, Ebo, Do);
    vROL(BCa, Ebo, 28);
    vxor(Egu, Egu, Du);
    vROL(BCe, Egu, 20);
    vxor(Eka, Eka, Da);
    vROL(BCi, Eka, 3);
    vxor(Eme, Eme, De);
    vROL(BCo, Eme, 45);
    vxor(Esi, Esi, Di);
    vROL(BCu, Esi, 61);
    vXNA(state[5], BCa, BCe, BCi);
    vXNA(state[6], BCe, BCi, BCo);
    vXNA(state[7], BCi, BCo, BCu);
    vXNA(state[8], BCo, BCu, BCa);
    vXNA(state[9], BCu, BCa, BCe);

    vxor(Ebe, Ebe, De);
    vROL(BCa, Ebe, 1);
    vxor(Egi, Egi, Di);
    vROL(BCe, Egi, 6);
    vxor(Eko, Eko, Do);
    vROL(BCi, Eko, 25);
    vxor(Emu, Emu, Du);
    vROL(BCo, Emu, 8);
    vxor(Esa, Esa, Da);
    vROL(BCu, Esa, 18);
    vXNA(state[10], BCa, BCe, BCi);
    vXNA(state[11], BCe, BCi, BCo);
    vXNA(state[12], BCi, BCo, BCu);
    vXNA(state[13], BCo, BCu, BCa);
    vXNA(state[14], BCu, BCa, BCe);

    vxor(Ebu, Ebu, Du);
    vROL(BCa, Ebu, 27);
    vxor(Ega, Ega, Da);
    vROL(BCe, Ega, 36);
    vxor(Eke, Eke, De);
    vROL(BCi, Eke, 10);
    vxor(Emi, Emi, Di);
    vROL(BCo, Emi, 15);
    vxor(Eso, Eso, Do);
    vROL(BCu, Eso, 56);
    vXNA(state[15], BCa, BCe, BCi);
    vXNA(state[16], BCe, BCi, BCo);
    vXNA(state[17], BCi, BCo, BCu);
    vXNA(state[18], BCo, BCu, BCa);
    vXNA(state[19], BCu, BCa, BCe);

    vxor(Ebi, Ebi, Di);
    vROL(BCa, Ebi, 62);
    vxor(Ego, Ego, Do);
    vROL(BCe, Ego, 55);
    vxor(Eku, Eku, Du);
    vROL(BCi, Eku, 39);
    vxor(Ema, Ema, Da);
    vROL(BCo, Ema, 41);
    vxor(Ese, Ese, De);
    vROL(BCu, Ese, 2);
    vXNA(state[20], BCa, BCe, BCi);
    vXNA(state[21], BCe, BCi, BCo);
    vXNA(state[22], BCi, BCo, BCu);
    vXNA(state[23], BCo, BCu, BCa);
    vXNA(state[24], BCu, BCa, BCe);
  }
}
/*************************************************
* Name:        keccakx2_absorb
*
* Description: Absorb step of Keccak;
*              non-incremental, starts by zeroeing the state.
*
* Arguments:   - uint64_t *s: pointer to (uninitialized) output Keccak state
*              - unsigned int r: rate in bytes (e.g., 168 for SHAKE128)
*              - const uint8_t *m: pointer to input to be absorbed into s
*              - size_t mlen: length of input in bytes
*              - uint8_t p: domain-separation byte for different
*                           Keccak-derived functions
**************************************************/
static inline __attribute__((always_inline))
void keccakx2_absorb(v128 s[25],
                                   unsigned int r,
                                   const uint8_t *in0,
                                   const uint8_t *in1,
                                   size_t inlen,
                                   uint8_t p)
{
  size_t i, pos = 0;

  // Declare SIMD registers
  v128 tmp, mask;
  uint64x1_t a, b;
  uint64x2_t a1, b1, atmp1, btmp1;
  uint64x2x2_t a2, b2, atmp2, btmp2;
  v128 Estate[25], Bstate[5], Dstate[5];
  // End

  for (i = 0; i < 25; ++i)
    s[i] = vdupq_n_u64(0);

  // Load in0[i] to register, then in1[i] to register, exchange them
  while (inlen >= r)
  {
    for (i = 0; i < r / 8 - 1; i += 4)
    {
      a2 = vld1q_u64_x2((uint64_t *)&in0[pos]);
      b2 = vld1q_u64_x2((uint64_t *)&in1[pos]);
      // BD = zip1(AB and CD)
      atmp2.val[0] = vzip1q_u64(a2.val[0], b2.val[0]);
      atmp2.val[1] = vzip1q_u64(a2.val[1], b2.val[1]);
      // AC = zip2(AB and CD)
      btmp2.val[0] = vzip2q_u64(a2.val[0], b2.val[0]);
      btmp2.val[1] = vzip2q_u64(a2.val[1], b2.val[1]);

      vxor(s[i + 0], s[i + 0], atmp2.val[0]);
      vxor(s[i + 1], s[i + 1], btmp2.val[0]);
      vxor(s[i + 2], s[i + 2], atmp2.val[1]);
      vxor(s[i + 3], s[i + 3], btmp2.val[1]);

      pos += 8 * 2 * 2;
    }
    // Last iteration
    i = r / 8 - 1;
    a = vld1_u64((uint64_t *)&in0[pos]);
    b = vld1_u64((uint64_t *)&in1[pos]);
    tmp = vcombine_u64(a, b);
    vxor(s[i], s[i], tmp);
    pos += 8;

    // KeccakF1600_StatePermutex2(s);
    // KeccakF1600_StatePermutex2_rename(s);
    KeccakF1600_StatePermutex2_macro(s, Estate, Bstate, Dstate);
    inlen -= r;
  }

  i = 0;
  while (inlen >= 16)
  {
    a1 = vld1q_u64((uint64_t *)&in0[pos]);
    b1 = vld1q_u64((uint64_t *)&in1[pos]);
    // BD = zip1(AB and CD)
    atmp1 = vzip1q_u64(a1, b1);
    // AC = zip2(AB and CD)
    btmp1 = vzip2q_u64(a1, b1);

    vxor(s[i + 0], s[i + 0], atmp1);
    vxor(s[i + 1], s[i + 1], btmp1);

    i += 2;
    pos += 8 * 2;
    inlen -= 8 * 2;
  }

  if (inlen >= 8)
  {
    a = vld1_u64((uint64_t *)&in0[pos]);
    b = vld1_u64((uint64_t *)&in1[pos]);
    tmp = vcombine_u64(a, b);
    vxor(s[i], s[i], tmp);

    i++;
    pos += 8;
    inlen -= 8;
  }

  if (inlen)
  {
    a = vld1_u64((uint64_t *)&in0[pos]);
    b = vld1_u64((uint64_t *)&in1[pos]);
    tmp = vcombine_u64(a, b);
    mask = vdupq_n_u64((1ULL << (8 * inlen)) - 1);
    tmp = vandq_u64(tmp, mask);
    vxor(s[i], s[i], tmp);
  }

  tmp = vdupq_n_u64((uint64_t)p << (8 * inlen));
  vxor(s[i], s[i], tmp);

  mask = vdupq_n_u64(1ULL << 63);
  vxor(s[r / 8 - 1], s[r / 8 - 1], mask);
}

/*************************************************
* Name:        keccak_squeezeblocks
*
* Description: Squeeze step of Keccak. Squeezes full blocks of r bytes each.
*              Modifies the state. Can be called multiple times to keep
*              squeezing, i.e., is incremental.
*
* Arguments:   - uint8_t *out: pointer to output blocks
*              - size_t nblocks: number of blocks to be squeezed (written to h)
*              - unsigned int r: rate in bytes (e.g., 168 for SHAKE128)
*              - uint64_t *s: pointer to input/output Keccak state
**************************************************/
static inline __attribute__((always_inline))
void keccakx2_squeezeblocks(uint8_t *out0,
                                          uint8_t *out1,
                                          size_t nblocks,
                                          unsigned int r,
                                          v128 s[25])
{
  unsigned int i;

  uint64x1_t a, b;
  uint64x2x2_t a2, b2;
  v128 Estate[25], Bstate[5], Dstate[5];

  while (nblocks > 0)
  {
    // KeccakF1600_StatePermutex2(s);
    // KeccakF1600_StatePermutex2_rename(s);
    KeccakF1600_StatePermutex2_macro(s, Estate, Bstate, Dstate);


    for (i = 0; i < r / 8 - 1; i += 4)
    {
      a2.val[0] = vuzp1q_u64(s[i], s[i + 1]);
      b2.val[0] = vuzp2q_u64(s[i], s[i + 1]);
      a2.val[1] = vuzp1q_u64(s[i + 2], s[i + 3]);
      b2.val[1] = vuzp2q_u64(s[i + 2], s[i + 3]);
      vst1q_u64_x2((uint64_t *)out0, a2);
      vst1q_u64_x2((uint64_t *)out1, b2);

      out0 += 32;
      out1 += 32;
    }

    i = r / 8 - 1;
    // Last iteration
    a = vget_low_u64(s[i]);
    b = vget_high_u64(s[i]);
    vst1_u64((uint64_t *)out0, a);
    vst1_u64((uint64_t *)out1, b);

    out0 += 8;
    out1 += 8;

    --nblocks;
  }
}

/*************************************************
* Name:        shake128x2_absorb
*
* Description: Absorb step of the SHAKE128 XOF.
*              non-incremental, starts by zeroeing the state.
*
* Arguments:   - keccakx2_state *state: pointer to (uninitialized) output
*                                     Keccak state
*              - const uint8_t *in:   pointer to input to be absorbed into s
*              - size_t inlen:        length of input in bytes
**************************************************/
void shake128x2_absorb(keccakx2_state *state,
                       const uint8_t *in0,
                       const uint8_t *in1,
                       size_t inlen)
{
  keccakx2_absorb(state->s, SHAKE128_RATE, in0, in1, inlen, 0x1F);
}

/*************************************************
* Name:        shake128_squeezeblocks
*
* Description: Squeeze step of SHAKE128 XOF. Squeezes full blocks of
*              SHAKE128_RATE bytes each. Modifies the state. Can be called
*              multiple times to keep squeezing, i.e., is incremental.
*
* Arguments:   - uint8_t *out:    pointer to output blocks
*              - size_t nblocks:  number of blocks to be squeezed
*                                 (written to output)
*              - keccakx2_state *s: pointer to input/output Keccak state
**************************************************/
void shake128x2_squeezeblocks(uint8_t *out0,
                              uint8_t *out1,
                              size_t nblocks,
                              keccakx2_state *state)
{
  keccakx2_squeezeblocks(out0, out1, nblocks, SHAKE128_RATE, state->s);
}

/*************************************************
* Name:        shake256_absorb
*
* Description: Absorb step of the SHAKE256 XOF.
*              non-incremental, starts by zeroeing the state.
*
* Arguments:   - keccakx2_state *s:   pointer to (uninitialized) output Keccak state
*              - const uint8_t *in: pointer to input to be absorbed into s
*              - size_t inlen:      length of input in bytes
**************************************************/
void shake256x2_absorb(keccakx2_state *state,
                       const uint8_t *in0,
                       const uint8_t *in1,
                       size_t inlen)
{
  keccakx2_absorb(state->s, SHAKE256_RATE, in0, in1, inlen, 0x1F);
}

/*************************************************
* Name:        shake256_squeezeblocks
*
* Description: Squeeze step of SHAKE256 XOF. Squeezes full blocks of
*              SHAKE256_RATE bytes each. Modifies the state. Can be called
*              multiple times to keep squeezing, i.e., is incremental.
*
* Arguments:   - uint8_t *out:    pointer to output blocks
*              - size_t nblocks:  number of blocks to be squeezed
*                                 (written to output)
*              - keccakx2_state *s: pointer to input/output Keccak state
**************************************************/
void shake256x2_squeezeblocks(uint8_t *out0,
                              uint8_t *out1,
                              size_t nblocks,
                              keccakx2_state *state)
{
  keccakx2_squeezeblocks(out0, out1, nblocks, SHAKE256_RATE, state->s);
}

/*************************************************
* Name:        shake128
*
* Description: SHAKE128 XOF with non-incremental API
*
* Arguments:   - uint8_t *out:      pointer to output
*              - size_t outlen:     requested output length in bytes
*              - const uint8_t *in: pointer to input
*              - size_t inlen:      length of input in bytes
**************************************************/
void shake128x2(uint8_t *out0,
                uint8_t *out1,
                size_t outlen,
                const uint8_t *in0,
                const uint8_t *in1,
                size_t inlen)
{
  unsigned int i;
  size_t nblocks = outlen / SHAKE128_RATE;
  uint8_t t[2][SHAKE128_RATE];
  keccakx2_state state;

  shake128x2_absorb(&state, in0, in1, inlen);
  shake128x2_squeezeblocks(out0, out1, nblocks, &state);

  out0 += nblocks * SHAKE128_RATE;
  out1 += nblocks * SHAKE128_RATE;
  outlen -= nblocks * SHAKE128_RATE;

  if (outlen)
  {
    shake128x2_squeezeblocks(t[0], t[1], 1, &state);
    for (i = 0; i < outlen; ++i)
    {
      out0[i] = t[0][i];
      out1[i] = t[1][i];
    }
  }
}

/*************************************************
* Name:        shake256
*
* Description: SHAKE256 XOF with non-incremental API
*
* Arguments:   - uint8_t *out:      pointer to output
*              - size_t outlen:     requested output length in bytes
*              - const uint8_t *in: pointer to input
*              - size_t inlen:      length of input in bytes
**************************************************/
void shake256x2(uint8_t *out0,
                uint8_t *out1,
                size_t outlen,
                const uint8_t *in0,
                const uint8_t *in1,
                size_t inlen)
{
  unsigned int i;
  size_t nblocks = outlen / SHAKE256_RATE;
  uint8_t t[2][SHAKE256_RATE];
  keccakx2_state state;

  shake256x2_absorb(&state, in0, in1, inlen);
  shake256x2_squeezeblocks(out0, out1, nblocks, &state);

  out0 += nblocks * SHAKE256_RATE;
  out1 += nblocks * SHAKE256_RATE;
  outlen -= nblocks * SHAKE256_RATE;

  if (outlen)
  {
    shake256x2_squeezeblocks(t[0], t[1], 1, &state);
    for (i = 0; i < outlen; ++i)
    {
      out0[i] = t[0][i];
      out1[i] = t[1][i];
    }
  }
}

/*************************************************
* Name:        sha3_256x2
*
* Description: SHA3-256 with non-incremental API
*
* Arguments:   - uint8_t *h:        pointer to output (32 bytes)
*              - const uint8_t *in: pointer to input
*              - size_t inlen:      length of input in bytes
**************************************************/
void sha3_256x2(uint8_t h1[32],
                uint8_t h2[32],
                const uint8_t *in1,
                const uint8_t *in2,
                size_t inlen)
{
  v128 s[25];
  uint8_t t1[SHA3_256_RATE];
  uint8_t t2[SHA3_256_RATE];

  keccakx2_absorb(s, SHA3_256_RATE, in1, in2, inlen, 0x06);
  keccakx2_squeezeblocks(t1, t2, 1, SHA3_256_RATE, s);

  uint8x16x2_t a, b;
  a = vld1q_u8_x2(t1);
  b = vld1q_u8_x2(t2);
  vst1q_u8_x2(h1, a);
  vst1q_u8_x2(h2, b);
}

/*************************************************
* Name:        sha3_512x2
*
* Description: SHA3-512 with non-incremental API
*
* Arguments:   - uint8_t *h:        pointer to output (64 bytes)
*              - const uint8_t *in: pointer to input
*              - size_t inlen:      length of input in bytes
**************************************************/
void sha3_512x2(uint8_t h1[64],
                uint8_t h2[64],
                const uint8_t *in1,
                const uint8_t *in2,
                size_t inlen)
{
  v128 s[25];
  uint8_t t1[SHA3_512_RATE];
  uint8_t t2[SHA3_512_RATE];

  keccakx2_absorb(s, SHA3_512_RATE, in1, in2, inlen, 0x06);
  keccakx2_squeezeblocks(t1, t2, 1, SHA3_512_RATE, s);

  uint8x16x4_t a, b;
  a = vld1q_u8_x4(t1);
  b = vld1q_u8_x4(t2);
  vst1q_u8_x4(h1, a);
  vst1q_u8_x4(h2, b);
}
