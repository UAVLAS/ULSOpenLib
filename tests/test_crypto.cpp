#include <cstdio>
#include <cstring>
#include "ULSCrypto.h"

static int fails = 0;
static void hex(const uint8_t*b,int n){for(int i=0;i<n;i++)printf("%02x",b[i]);}
static void check(const char*name,const uint8_t*got,const char*want,int n){
  char buf[128]; for(int i=0;i<n;i++) snprintf(buf+2*i,3,"%02x",got[i]);
  bool ok = strcmp(buf,want)==0;
  printf("%-28s %s\n", name, ok?"PASS":"FAIL");
  if(!ok){ printf("   got  %s\n   want %s\n", buf, want); fails++; }
}
static void unhex(const char*s, uint8_t*o){
  int n=strlen(s)/2; for(int i=0;i<n;i++){unsigned v;sscanf(s+2*i,"%2x",&v);o[i]=(uint8_t)v;}
}

int main(){
  // FIPS-197 C.1  AES-128
  uint8_t k[16], pt[16], ct[16];
  unhex("000102030405060708090a0b0c0d0e0f", k);
  unhex("00112233445566778899aabbccddeeff", pt);
  ulsCryptoAes128EncryptSw(k, pt, ct);
  check("FIPS-197 AES-128", ct, "69c4e0d86a7b0430d8cdb78070b4c55a", 16);

  // aliasing in==out must work
  uint8_t buf[16]; memcpy(buf, pt, 16);
  ulsCryptoAes128EncryptSw(k, buf, buf);
  check("AES in/out aliasing", buf, "69c4e0d86a7b0430d8cdb78070b4c55a", 16);

  // RFC 4493 CMAC-AES128
  uint8_t ck[16], msg[64], mac[16];
  unhex("2b7e151628aed2a6abf7158809cf4f3c", ck);
  unhex("6bc1bee22e409f96e93d7e117393172a"
        "ae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52ef"
        "f69f2445df4f9b17ad2b417be66c3710", msg);

  ulsCryptoCmac(ck, msg, 0, mac);
  check("RFC 4493 CMAC len=0",  mac, "bb1d6929e95937287fa37d129b756746", 16);
  ulsCryptoCmac(ck, msg, 16, mac);
  check("RFC 4493 CMAC len=16", mac, "070a16b46b4d4144f79bdd9dd04a287c", 16);
  ulsCryptoCmac(ck, msg, 40, mac);
  check("RFC 4493 CMAC len=40", mac, "dfa66747de9ae63030ca32611497c827", 16);
  ulsCryptoCmac(ck, msg, 64, mac);
  check("RFC 4493 CMAC len=64", mac, "51f0bebf7e3b9d92fc49741779363cfe", 16);

  // constant-time compare
  uint8_t a[4]={1,2,3,4}, b[4]={1,2,3,4}, c[4]={1,2,3,5};
  printf("%-28s %s\n","ulsCryptoEqual",
    (ulsCryptoEqual(a,b,4) && !ulsCryptoEqual(a,c,4))?"PASS":"FAIL");
  if(!(ulsCryptoEqual(a,b,4) && !ulsCryptoEqual(a,c,4))) fails++;

  // PRNG: refuses unseeded, never repeats, differs per counter
  ULSCryptoPrng p;
  uint8_t r1[16], r2[16];
  bool unseededRefused = !p.get(r1,16);
  p.seed(ck, 7);
  bool got1 = p.get(r1,16);
  bool got2 = p.get(r2,16);
  bool differs = memcmp(r1,r2,16)!=0;
  bool ctrMoved = (p.counter()==9);
  printf("%-28s %s\n","PRNG seed/step/refuse",
    (unseededRefused&&got1&&got2&&differs&&ctrMoved)?"PASS":"FAIL");
  if(!(unseededRefused&&got1&&got2&&differs&&ctrMoved)) fails++;

  // same counter reproduces - which is exactly why it must be persisted
  ULSCryptoPrng q; q.seed(ck, 7);
  uint8_t r3[16]; q.get(r3,16);
  printf("%-28s %s\n","PRNG counter reuse repeats", memcmp(r1,r3,16)==0?"PASS":"FAIL");
  if(memcmp(r1,r3,16)!=0) fails++;

  // >16 byte draw
  uint8_t big[48]; ULSCryptoPrng z; z.seed(ck,1);
  bool bigok = z.get(big,48) && memcmp(big,big+16,16)!=0 && memcmp(big+16,big+32,16)!=0;
  printf("%-28s %s\n","PRNG multi-block draw", bigok?"PASS":"FAIL");
  if(!bigok) fails++;

  // key derivation is deterministic and label-separated
  uint8_t serial[4]={0xde,0xad,0xbe,0xef};
  uint8_t kp[16],kp2[16],ko[16];
  ulsCryptoDeriveKey(ck,"peer",serial,4,kp);
  ulsCryptoDeriveKey(ck,"peer",serial,4,kp2);
  ulsCryptoDeriveKey(ck,"operator",serial,4,ko);
  bool dok = memcmp(kp,kp2,16)==0 && memcmp(kp,ko,16)!=0;
  printf("%-28s %s\n","deriveKey det + separated", dok?"PASS":"FAIL");
  if(!dok) fails++;
  printf("   peer key = "); hex(kp,16); printf("\n");

  printf("\n%s\n", fails? "FAILURES" : "all vectors pass");
  return fails!=0;
}
