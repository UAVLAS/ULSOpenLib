/**
 *  Copyright: 2026 by UAVLAS  <www.uavlas.com>
 *
 * This file is part of UAVLAS project applications.
 *
 * This is free software: you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * @license LGPL-3.0+ <https://spdx.org/licenses/LGPL-3.0+>
 *
 * AES-128 encryption and CMAC-AES128. Verified against the FIPS-197 example
 * vector and all four RFC 4493 CMAC vectors; see tests/test_crypto.cpp.
 *
 * The cipher is the compact, table-free-but-for-the-S-box form: one 256-byte
 * S-box and no T-tables.
 */
#include "ULSCrypto.h"

static const uint8_t _sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
    0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
    0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed,
    0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
    0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
    0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
    0xb0, 0x54, 0xbb, 0x16};

static const uint8_t _rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10,
                                  0x20, 0x40, 0x80, 0x1b, 0x36};

/* Multiply by x in GF(2^8) with the AES polynomial. Branch-free: the mask is
 * built from the high bit rather than tested, so the timing does not depend on
 * the data. */
static inline uint8_t _xtime(uint8_t x) {
  return (uint8_t)((uint8_t)(x << 1) ^ (uint8_t)((uint8_t)(0u - (x >> 7)) & 0x1bu));
}

/* State is 16 bytes in column-major order, byte i = row (i%4), column (i/4) -
 * the order the bytes arrive in, so no transposition is needed anywhere. */
static void _expandKey(const uint8_t key[16], uint8_t rk[176]) {
  memcpy(rk, key, 16);
  for (uint32_t i = 4; i < 44; i++) {
    uint8_t t[4];
    memcpy(t, &rk[(i - 1) * 4], 4);
    if ((i & 3u) == 0u) {
      uint8_t tmp = t[0]; /* RotWord */
      t[0] = _sbox[t[1]];
      t[1] = _sbox[t[2]];
      t[2] = _sbox[t[3]];
      t[3] = _sbox[tmp];
      t[0] ^= _rcon[(i >> 2) - 1];
    }
    for (uint32_t j = 0; j < 4; j++) {
      rk[i * 4 + j] = rk[(i - 4) * 4 + j] ^ t[j];
    }
  }
}

void ulsCryptoAes128EncryptSw(const uint8_t key[16], const uint8_t in[16],
                              uint8_t out[16]) {
  uint8_t rk[176];
  uint8_t s[16];

  _expandKey(key, rk);
  memcpy(s, in, 16);

  for (uint32_t i = 0; i < 16; i++) s[i] ^= rk[i]; /* round 0 */

  for (uint32_t round = 1; round <= 10; round++) {
    uint8_t t[16];

    /* SubBytes and ShiftRows together: row r moves left by r columns, so the
     * byte landing at (r, c) comes from (r, c + r). */
    for (uint32_t c = 0; c < 4; c++) {
      for (uint32_t r = 0; r < 4; r++) {
        t[r + 4 * c] = _sbox[s[r + 4 * ((c + r) & 3u)]];
      }
    }

    if (round != 10) { /* MixColumns, omitted in the last round */
      for (uint32_t c = 0; c < 4; c++) {
        uint8_t *p = &t[4 * c];
        uint8_t a0 = p[0], a1 = p[1], a2 = p[2], a3 = p[3];
        uint8_t all = (uint8_t)(a0 ^ a1 ^ a2 ^ a3);
        p[0] = (uint8_t)(a0 ^ all ^ _xtime((uint8_t)(a0 ^ a1)));
        p[1] = (uint8_t)(a1 ^ all ^ _xtime((uint8_t)(a1 ^ a2)));
        p[2] = (uint8_t)(a2 ^ all ^ _xtime((uint8_t)(a2 ^ a3)));
        p[3] = (uint8_t)(a3 ^ all ^ _xtime((uint8_t)(a3 ^ a0)));
      }
    }

    for (uint32_t i = 0; i < 16; i++) s[i] = t[i] ^ rk[round * 16 + i];
  }

  memcpy(out, s, 16);
}

bool ulsCryptoRandomNone(uint8_t *buf, uint32_t len) {
  (void)buf;
  (void)len;
  return false;
}

/* Double a 128-bit value in GF(2^128), RFC 4493 section 2.3. Constant time for
 * the same reason _xtime is. */
static void _dbl(const uint8_t in[16], uint8_t out[16]) {
  uint8_t carry = (uint8_t)(in[0] >> 7);
  for (uint32_t i = 0; i < 15; i++) {
    out[i] = (uint8_t)((in[i] << 1) | (in[i + 1] >> 7));
  }
  out[15] = (uint8_t)(in[15] << 1);
  out[15] ^= (uint8_t)((uint8_t)(0u - carry) & 0x87u);
}

void ulsCryptoCmac(const uint8_t key[16], const uint8_t *msg, uint32_t len,
                   uint8_t mac[16]) {
  uint8_t l[16], k1[16], k2[16];
  uint8_t x[16], block[16];

  memset(l, 0, 16);
  ULS_CRYPTO_AES128_ENCRYPT(key, l, l);
  _dbl(l, k1);
  _dbl(k1, k2);

  uint32_t n = (len + 15u) / 16u;
  bool complete;
  if (n == 0u) {
    n = 1u;
    complete = false;
  } else {
    complete = ((len & 15u) == 0u);
  }

  memset(x, 0, 16);

  for (uint32_t i = 0; i < n - 1u; i++) {
    for (uint32_t j = 0; j < 16; j++) x[j] ^= msg[i * 16u + j];
    ULS_CRYPTO_AES128_ENCRYPT(key, x, x);
  }

  /* Last block: whole and XORed with K1, or padded with 0x80 00.. and K2. */
  uint32_t done = (n - 1u) * 16u;
  if (complete) {
    memcpy(block, &msg[done], 16);
    for (uint32_t j = 0; j < 16; j++) block[j] ^= k1[j];
  } else {
    uint32_t rem = len - done;
    memset(block, 0, 16);
    if (rem) memcpy(block, &msg[done], rem);
    block[rem] = 0x80;
    for (uint32_t j = 0; j < 16; j++) block[j] ^= k2[j];
  }

  for (uint32_t j = 0; j < 16; j++) x[j] ^= block[j];
  ULS_CRYPTO_AES128_ENCRYPT(key, x, mac);
}

bool ulsCryptoEqual(const uint8_t *a, const uint8_t *b, uint32_t len) {
  uint8_t diff = 0;
  for (uint32_t i = 0; i < len; i++) diff |= (uint8_t)(a[i] ^ b[i]);
  return diff == 0u;
}

void ulsCryptoDeriveKey(const uint8_t key[16], const char *label,
                        const uint8_t *context, uint32_t contextLen,
                        uint8_t out[16]) {
  uint8_t buf[64];
  uint32_t n = 0;

  while ((label != nullptr) && (label[n] != '\0') && (n < 16u)) {
    buf[n] = (uint8_t)label[n];
    n++;
  }
  if (contextLen > (sizeof(buf) - n)) contextLen = sizeof(buf) - n;
  if (contextLen) memcpy(&buf[n], context, contextLen);

  ulsCryptoCmac(key, buf, n + contextLen, out);
}

void ULSCryptoPrng::seed(const uint8_t seedKey[16], uint32_t counter) {
  memcpy(_key, seedKey, 16);
  _ctr = counter;
  _seeded = true;
}

bool ULSCryptoPrng::get(uint8_t *buf, uint32_t len) {
  if (!_seeded) return false;

  uint32_t idx = 0;
  while (len) {
    uint8_t in[14];
    uint8_t block[16];

    memcpy(in, "ULSRND", 6);
    in[6] = (uint8_t)(_ctr);
    in[7] = (uint8_t)(_ctr >> 8);
    in[8] = (uint8_t)(_ctr >> 16);
    in[9] = (uint8_t)(_ctr >> 24);
    in[10] = (uint8_t)(idx);
    in[11] = (uint8_t)(idx >> 8);
    in[12] = (uint8_t)(idx >> 16);
    in[13] = (uint8_t)(idx >> 24);

    ulsCryptoCmac(_key, in, sizeof(in), block);

    uint32_t take = (len > 16u) ? 16u : len;
    memcpy(&buf[idx * 16u], block, take);
    len -= take;
    idx++;
  }

  /* One draw, one counter step. The caller persists counter() so the next boot
   * starts beyond every value this one used. */
  _ctr++;
  return true;
}
