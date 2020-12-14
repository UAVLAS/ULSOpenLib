#ifndef UDEBUG_H
#define UDEBUG_H

#include <stdio.h>
#include <stdarg.h>

#define ULS_DEBUG

#ifdef __cplusplus
extern "C" {
#endif
void udebugTickHandler();
void udebugElspsed(unsigned int etaime);
void uDebug(const char* msg, ...);
void uError(const char *file, int line,const char* msg, ...);

#ifdef __cplusplus
}
#endif

#ifdef ULS_DBUG_MSG
#define DEBUG_MSG(MSG,...) uDebug(MSG,##__VA_ARGS__)
#else
#define DEBUG_MSG(MSG,...) void()

#endif

#ifdef ULS_DBUG_ERROR
#define DEBUG_ERROR(FILE,LINE,MSG,...) uError(FILE,LINE,MSG,##__VA_ARGS__)
#else
#define DEBUG_ERROR(FILE,LINE,MSG,...) (void)FILE;(void)LINE;(void)MSG;
#endif


#endif // UDEBUG_H
