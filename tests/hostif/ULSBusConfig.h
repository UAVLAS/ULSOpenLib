#ifndef ULSBUSCONFIG_H
#define ULSBUSCONFIG_H
/* The tests exist to exercise the join path, so they always build it in. */
#define ULSBUS_AUTH

#define ULS_ENTER_CRITICAL void()
#define ULS_EXIT_CRITICAL void()
#define ULS_DISABLE_IRQ void()
#define ULS_ENABLE_IRQ void()
/* Two identities, switched per instance by the harness. */
extern unsigned int g_uid0;
#define __DEVICE_UNIC_ID0 (g_uid0)
#define __DEVICE_UNIC_ID1 0x11111111u
#define __DEVICE_UNIC_ID2 0x22222222u
#define __DEVICE_UNIC_ID3 0x33333333u
/* Deterministic "random" for the harness only; replaced per test. */
extern bool hostRandom(unsigned char *buf, unsigned int len);
#define ULS_CRYPTO_RANDOM hostRandom
#endif
