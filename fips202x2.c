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
#include <arm_neon.h>
#include <stddef.h>
#include "fips202x2.h"

#define NROUNDS 24
#define _APPLE_M1_ 1

// Define NEON operation

// Bitwise-XOR: c = a ^ b
#define vxor(c, a, b) c = veorq_u64(a, b);

#if _APPLE_M1_ == 1

/*
 * At least ARMv8.2-sha3 supported
 */

// Xor chain: out = a ^ b ^ c ^ d ^ e
#define vXOR5(out, a, b, c, d, e) \
  out = veor3q_u64(a, b, c);      \
  out = veor3q_u64(out, d, e);

// Rotate left by 1 bit, then XOR: a ^ ROL(b)
#define vRXOR(c, a, b) c = vrax1q_u64(a, b);

// XOR then Rotate by n bit: c = ROL(a^b, n)
#define vXORR(c, a, b, n) c = vxarq_u64(a, b, n);

// Xor Not And: out = a ^ ( (~b) & c)
#define vXNA(out, a, b, c) out = vbcaxq_u64(a, c, b);

#else

// Rotate left by n bit
#define vROL(out, a, offset)      \
  out = vshlq_n_u64(a, (offset)); \
  out = vsriq_n_u64(out, a, 64 - (offset));

// Xor chain: out = a ^ b ^ c ^ d ^ e
#define vXOR5(out, a, b, c, d, e) \
  out = veorq_u64(a, b);          \
  out = veorq_u64(out, c);        \
  out = veorq_u64(out, d);        \
  out = veorq_u64(out, e);

// Xor Not And: out = a ^ ( (~b) & c)
#define vXNA(out, a, b, c) \
  out = vbicq_u64(c, b);   \
  out = veorq_u64(out, a);

#define vRXOR(c, a, b) \
  vROL(c, b, 1);       \
  vxor(c, c, a);

#define vXORR(c, a, b, n) \
  a = veorq_u64(a, b);    \
  vROL(c, a, 64 - n);

#endif

// End

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

/*************************************************
 * Name:        KeccakF1600_StatePermutex2
 *
 * Description: The Keccak F1600 Permutation
 *
 * Arguments:   - v128 *state: pointer to input/output Keccak state
 **************************************************/
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

  // copyFromState(A, state)
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
    vXOR5(BCa, Aba, Aga, Aka, Ama, Asa);
    vXOR5(BCe, Abe, Age, Ake, Ame, Ase);
    vXOR5(BCi, Abi, Agi, Aki, Ami, Asi);
    vXOR5(BCo, Abo, Ago, Ako, Amo, Aso);
    vXOR5(BCu, Abu, Agu, Aku, Amu, Asu);

    vRXOR(Da, BCu, BCe);
    vRXOR(De, BCa, BCi);
    vRXOR(Di, BCe, BCo);
    vRXOR(Do, BCi, BCu);
    vRXOR(Du, BCo, BCa);

    vxor(Aba, Aba, Da);
    vXORR(BCe, Age, De, 20);
    vXORR(BCi, Aki, Di, 21);
    vXORR(BCo, Amo, Do, 43);
    vXORR(BCu, Asu, Du, 50);

    vXNA(Eba, Aba, BCe, BCi);
    vxor(Eba, Eba, vdupq_n_u64(neon_KeccakF_RoundConstants[round]));
    vXNA(Ebe, BCe, BCi, BCo);
    vXNA(Ebi, BCi, BCo, BCu);
    vXNA(Ebo, BCo, BCu, Aba);
    vXNA(Ebu, BCu, Aba, BCe);

    vXORR(BCa, Abo, Do, 36);
    vXORR(BCe, Agu, Du, 44);
    vXORR(BCi, Aka, Da, 61);
    vXORR(BCo, Ame, De, 19);
    vXORR(BCu, Asi, Di, 3);

    vXNA(Ega, BCa, BCe, BCi);
    vXNA(Ege, BCe, BCi, BCo);
    vXNA(Egi, BCi, BCo, BCu);
    vXNA(Ego, BCo, BCu, BCa);
    vXNA(Egu, BCu, BCa, BCe);

    vXORR(BCa, Abe, De, 63);
    vXORR(BCe, Agi, Di, 58);
    vXORR(BCi, Ako, Do, 39);
    vXORR(BCo, Amu, Du, 56);
    vXORR(BCu, Asa, Da, 46);

    vXNA(Eka, BCa, BCe, BCi);
    vXNA(Eke, BCe, BCi, BCo);
    vXNA(Eki, BCi, BCo, BCu);
    vXNA(Eko, BCo, BCu, BCa);
    vXNA(Eku, BCu, BCa, BCe);

    vXORR(BCa, Abu, Du, 37);
    vXORR(BCe, Aga, Da, 28);
    vXORR(BCi, Ake, De, 54);
    vXORR(BCo, Ami, Di, 49);
    vXORR(BCu, Aso, Do, 8);

    vXNA(Ema, BCa, BCe, BCi);
    vXNA(Eme, BCe, BCi, BCo);
    vXNA(Emi, BCi, BCo, BCu);
    vXNA(Emo, BCo, BCu, BCa);
    vXNA(Emu, BCu, BCa, BCe);

    vXORR(BCa, Abi, Di, 2);
    vXORR(BCe, Ago, Do, 9);
    vXORR(BCi, Aku, Du, 25);
    vXORR(BCo, Ama, Da, 23);
    vXORR(BCu, Ase, De, 62);

    vXNA(Esa, BCa, BCe, BCi);
    vXNA(Ese, BCe, BCi, BCo);
    vXNA(Esi, BCi, BCo, BCu);
    vXNA(Eso, BCo, BCu, BCa);
    vXNA(Esu, BCu, BCa, BCe);

    // Next Round

    //    prepareTheta
    vXOR5(BCa, Eba, Ega, Eka, Ema, Esa);
    vXOR5(BCe, Ebe, Ege, Eke, Eme, Ese);
    vXOR5(BCi, Ebi, Egi, Eki, Emi, Esi);
    vXOR5(BCo, Ebo, Ego, Eko, Emo, Eso);
    vXOR5(BCu, Ebu, Egu, Eku, Emu, Esu);

    // thetaRhoPiChiIotaPrepareTheta(round+1, E, A)
    vRXOR(Da, BCu, BCe);
    vRXOR(De, BCa, BCi);
    vRXOR(Di, BCe, BCo);
    vRXOR(Do, BCi, BCu);
    vRXOR(Du, BCo, BCa);

    vxor(Eba, Eba, Da);
    vXORR(BCe, Ege, De, 20);
    vXORR(BCi, Eki, Di, 21);
    vXORR(BCo, Emo, Do, 43);
    vXORR(BCu, Esu, Du, 50);

    vXNA(Aba, Eba, BCe, BCi);
    vxor(Aba, Aba, vdupq_n_u64(neon_KeccakF_RoundConstants[round + 1]));
    vXNA(Abe, BCe, BCi, BCo);
    vXNA(Abi, BCi, BCo, BCu);
    vXNA(Abo, BCo, BCu, Eba);
    vXNA(Abu, BCu, Eba, BCe);

    vXORR(BCa, Ebo, Do, 36);
    vXORR(BCe, Egu, Du, 44);
    vXORR(BCi, Eka, Da, 61);
    vXORR(BCo, Eme, De, 19);
    vXORR(BCu, Esi, Di, 3);

    vXNA(Aga, BCa, BCe, BCi);
    vXNA(Age, BCe, BCi, BCo);
    vXNA(Agi, BCi, BCo, BCu);
    vXNA(Ago, BCo, BCu, BCa);
    vXNA(Agu, BCu, BCa, BCe);

    vXORR(BCa, Ebe, De, 63);
    vXORR(BCe, Egi, Di, 58);
    vXORR(BCi, Eko, Do, 39);
    vXORR(BCo, Emu, Du, 56);
    vXORR(BCu, Esa, Da, 46);

    vXNA(Aka, BCa, BCe, BCi);
    vXNA(Ake, BCe, BCi, BCo);
    vXNA(Aki, BCi, BCo, BCu);
    vXNA(Ako, BCo, BCu, BCa);
    vXNA(Aku, BCu, BCa, BCe);

    vXORR(BCa, Ebu, Du, 37);
    vXORR(BCe, Ega, Da, 28);
    vXORR(BCi, Eke, De, 54);
    vXORR(BCo, Emi, Di, 49);
    vXORR(BCu, Eso, Do, 8);

    vXNA(Ama, BCa, BCe, BCi);
    vXNA(Ame, BCe, BCi, BCo);
    vXNA(Ami, BCi, BCo, BCu);
    vXNA(Amo, BCo, BCu, BCa);
    vXNA(Amu, BCu, BCa, BCe);

    vXORR(BCa, Ebi, Di, 2);
    vXORR(BCe, Ego, Do, 9);
    vXORR(BCi, Eku, Du, 25);
    vXORR(BCo, Ema, Da, 23);
    vXORR(BCu, Ese, De, 62);

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

/*************************************************
 * Name:        keccakx2_absorb
 *
 * Description: Absorb step of Keccak;
 *              non-incremental, starts by zeroeing the state.
 *
 * Arguments:   - v128 *s: pointer to (uninitialized) output Keccak state
 *              - unsigned int r: rate in bytes (e.g., 168 for SHAKE128)
 *              - const uint8_t *in0, *in1: pointer to input to be absorbed into s
 *              - size_t mlen: length of input in bytes
 *              - uint8_t p: domain-separation byte for different
 *                           Keccak-derived functions
 **************************************************/
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

    KeccakF1600_StatePermutex2(s);
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
 * Name:        keccakx2_squeezeblocks
 *
 * Description: Squeeze step of Keccak. Squeezes full blocks of r bytes each.
 *              Modifies the state. Can be called multiple times to keep
 *              squeezing, i.e., is incremental.
 *
 * Arguments:   - uint8_t *out, *out1: pointer to output blocks
 *              - size_t nblocks: number of blocks to be squeezed (written to h)
 *              - unsigned int r: rate in bytes (e.g., 168 for SHAKE128)
 *              - v128 *s: pointer to input/output Keccak state
 **************************************************/
void keccakx2_squeezeblocks(uint8_t *out0,
                            uint8_t *out1,
                            size_t nblocks,
                            unsigned int r,
                            v128 s[25])
{
  unsigned int i;

  uint64x1_t a, b;
  uint64x2x2_t a2, b2;

  while (nblocks > 0)
  {
    KeccakF1600_StatePermutex2(s);

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
 *              - const uint8_t *in0, *in1: pointer to input to be absorbed into s
 *              - size_t inlen: length of input in bytes
 **************************************************/
void shake128x2_absorb(keccakx2_state *state,
                       const uint8_t *in0,
                       const uint8_t *in1,
                       size_t inlen)
{
  keccakx2_absorb(state->s, SHAKE128_RATE, in0, in1, inlen, 0x1F);
}

/*************************************************
 * Name:        shake128x2_squeezeblocks
 *
 * Description: Squeeze step of SHAKE128 XOF. Squeezes full blocks of
 *              SHAKE128_RATE bytes each. Modifies the state. Can be called
 *              multiple times to keep squeezing, i.e., is incremental.
 *
 * Arguments:   - uint8_t *out0, *out1: pointer to output blocks
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
 * Name:        shake256x2_absorb
 *
 * Description: Absorb step of the SHAKE256 XOF.
 *              non-incremental, starts by zeroeing the state.
 *
 * Arguments:   - keccakx2_state *s: pointer to (uninitialized) output Keccak state
 *              - const uint8_t *in0, *in1: pointer to input to be absorbed into s
 *              - size_t inlen: length of input in bytes
 **************************************************/
void shake256x2_absorb(keccakx2_state *state,
                       const uint8_t *in0,
                       const uint8_t *in1,
                       size_t inlen)
{
  keccakx2_absorb(state->s, SHAKE256_RATE, in0, in1, inlen, 0x1F);
}

/*************************************************
 * Name:        shake256x2_squeezeblocks
 *
 * Description: Squeeze step of SHAKE256 XOF. Squeezes full blocks of
 *              SHAKE256_RATE bytes each. Modifies the state. Can be called
 *              multiple times to keep squeezing, i.e., is incremental.
 *
 * Arguments:   - uint8_t *out0, *out1: pointer to output blocks
 *              - size_t nblocks: number of blocks to be squeezed
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
 * Name:        shake128x2
 *
 * Description: SHAKE128 XOF with non-incremental API
 *
 * Arguments:   - uint8_t *out0, *out1: pointer to output
 *              - size_t outlen: requested output length in bytes
 *              - const uint8_t *in0, *in1: pointer to input
 *              - size_t inlen: length of input in bytes
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
 * Name:        shake256x2
 *
 * Description: SHAKE256 XOF with non-incremental API
 *
 * Arguments:   - uint8_t *out0, *out1: pointer to output
 *              - size_t outlen: requested output length in bytes
 *              - const uint8_t *in0, *in1: pointer to input
 *              - size_t inlen: length of input in bytes
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
 * Arguments:   - uint8_t *h1, *h2: pointer to output (32 bytes)
 *              - const uint8_t *in0, *in1: pointer to input
 *              - size_t inlen: length of input in bytes
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
 * Arguments:   - uint8_t *h0, *h1: pointer to output (64 bytes)
 *              - const uint8_t *in0, *in1: pointer to input
 *              - size_t inlen: length of input in bytes
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
