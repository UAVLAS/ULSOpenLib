#ifndef UDEBUG_H
#define UDEBUG_H

#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif
void udebugTickHandler();
void udebugElspsed(unsigned int etaime);
void uDebug(const char* msg, ...);
void uError(const char *file, int line,const char* msg, ...);
void uDebugPacket(const char* msg,const char* msg2, uint8_t *buf,uint32_t len);

#ifdef __cplusplus
}
#endif

#ifdef ULS_DEBUG
#define DEBUG_MSG(MSG,...) uDebug(MSG,##__VA_ARGS__)
#define DEBUG_ERROR(FILE,LINE,MSG,...) uError(FILE,LINE,MSG,##__VA_ARGS__)

#else
#define DEBUG_MSG(MSG,...) (void)MSG;
#define DEBUG_ERROR(FILE,LINE,MSG,...) {(void)FILE;(void)LINE;(void)MSG;}
#endif

#ifdef ULS_DEBUG_FULL
#define DEBUG_PACKET(MSG,MSG2,BUF,LEN) uDebugPacket(MSG,MSG2,BUF,LEN)
#else
#define DEBUG_PACKET(MSG,MSG2,BUF,LEN) {(void)MSG;(void)MSG2;(void)BUF;(void)LEN;}
#endif


#ifndef ULS_DBUG_MSG
#define ULS_DBUG_MSG(x) printf("%s\n",x);
#endif

#endif // UDEBUG_H

