#ifndef UDEBUG_H
#define UDEBUG_H

#include <stdio.h>
#include <stdarg.h>

#define ULS_DEBUG
#ifdef __cplusplus
extern "C" {
#endif
void udebugTickHandler();
void uDebug(const char* msg, ...);
void uError(const char *file, int line,const char* msg, ...);

#ifdef __cplusplus
}
#endif

#endif // UDEBUG_H
