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
 * AES-128 and CMAC for ULS bus authorization, with the two hardware-dependent
 * pieces - the block cipher and the entropy source - replaceable per platform.
 *
 * Everything here works with no platform support at all. A platform that has
 * an AES peripheral or a TRNG says so in its ULSBusConfig.h and the software
 * paths drop out at compile time:
 *
 *   #define ULS_CRYPTO_AES128_ENCRYPT  myHardwareAes
 *   #define ULS_CRYPTO_RANDOM          myHardwareTrng
 *
 * The signatures must match ulsCryptoAes128EncryptSw and ulsCryptoRandomNone
 * below. Nothing in ULSOpenLib refers to the software versions by name, so an
 * override replaces them everywhere at once.
 */
#ifndef ULSCRYPTO_H
#define ULSCRYPTO_H

#include <inttypes.h>
#include <string.h>

/*
 * Where a platform declares its overrides. Optional: the host tests build this
 * file with no platform at all, and every device in the tree already supplies
 * a ULSBusConfig.h for exactly this sort of thing (ULS_ENTER_CRITICAL and the
 * rest). Pulling it in HERE rather than only from ULSBusTypes.h matters -
 * without it ULSCrypto.cpp would compile its own CMAC against the software
 * cipher while everyone else used the hardware one.
 */
#if defined(__has_include)
#if __has_include("ULSBusConfig.h")
#include "ULSBusConfig.h"
#endif
#endif

#define ULS_AES_BLOCK_SIZE 16
#define ULS_AES_KEY_SIZE 16
#define ULS_CMAC_SIZE 16

/*
 * Encrypt one block. Encryption only - CMAC never decrypts, and leaving the
 * inverse cipher out saves both the code and the inverse tables.
 * in and out may alias.
 */
void ulsCryptoAes128EncryptSw(const uint8_t key[ULS_AES_KEY_SIZE],
                              const uint8_t in[ULS_AES_BLOCK_SIZE],
                              uint8_t out[ULS_AES_BLOCK_SIZE]);

/*
 * The default entropy source: there isn't one. Returns false and leaves buf
 * untouched, so a platform that neither provides a TRNG nor seeds
 * ULSCryptoPrng cannot silently authenticate with predictable nonces - it
 * fails to authenticate at all, which is the safe direction.
 *
 * The STM32G0B1 is exactly this case: no RNG peripheral and no AES. Seed a
 * ULSCryptoPrng from the device secret and a counter that survives reset.
 */
bool ulsCryptoRandomNone(uint8_t *buf, uint32_t len);

#ifndef ULS_CRYPTO_AES128_ENCRYPT
#define ULS_CRYPTO_AES128_ENCRYPT ulsCryptoAes128EncryptSw
#endif

#ifndef ULS_CRYPTO_RANDOM
#define ULS_CRYPTO_RANDOM ulsCryptoRandomNone
#endif

/*
 * CMAC-AES128, RFC 4493. Built on whichever block cipher is configured, so a
 * hardware AES accelerates this too.
 */
void ulsCryptoCmac(const uint8_t key[ULS_AES_KEY_SIZE], const uint8_t *msg,
                   uint32_t len, uint8_t mac[ULS_CMAC_SIZE]);

/*
 * Constant-time compare, for MACs. memcmp returns early on the first differing
 * byte and so leaks how much of a forged MAC was right, which is enough to
 * find the rest a byte at a time.
 */
bool ulsCryptoEqual(const uint8_t *a, const uint8_t *b, uint32_t len);

/*
 * Derive one key from another: out = CMAC(key, label || context).
 * This is what turns one fleet key into a per-device, per-role secret.
 */
void ulsCryptoDeriveKey(const uint8_t key[ULS_AES_KEY_SIZE], const char *label,
                        const uint8_t *context, uint32_t contextLen,
                        uint8_t out[ULS_AES_KEY_SIZE]);

/*
 * A counter-mode PRNG for parts with no TRNG.
 *
 * Output is CMAC(seedKey, "ULSRND" || counter || block index), so it is
 * unpredictable to anyone without the seed key, and never repeats as long as
 * the counter never repeats. That last condition is the whole security of it:
 *
 *   THE COUNTER MUST BE PERSISTED ACROSS RESET AND RESTORED AHEAD OF THE LAST
 *   VALUE USED.
 *
 * Restoring a stale counter replays nonces, and a replayed nonce replays the
 * proof that answered it. Persist ahead in chunks - save counter + N, use up
 * to N draws before saving again - so a reset costs a jump forward and never
 * a step back. On the EIGC-G3 that store is the F-RAM.
 */
class ULSCryptoPrng {
 public:
  ULSCryptoPrng() : _ctr(0), _seeded(false) { memset(_key, 0, sizeof(_key)); }

  /* seedKey should be a device secret, not the fleet key. */
  void seed(const uint8_t seedKey[ULS_AES_KEY_SIZE], uint32_t counter);

  bool get(uint8_t *buf, uint32_t len);

  /* Persist this, and reseed from a value strictly greater than it. */
  uint32_t counter() const { return _ctr; }
  bool seeded() const { return _seeded; }

 private:
  uint8_t _key[ULS_AES_KEY_SIZE];
  uint32_t _ctr;
  bool _seeded;
};

#endif  // ULSCRYPTO_H
