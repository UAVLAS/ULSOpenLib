#include "udebug.h"

#ifdef __cplusplus
extern "C" {
#endif

static unsigned int _startup_time = 0;
char tmpStr[1024];
void udebugTickHandler()
{
    _startup_time++;
}
void uDebug(const char* msg, ...)
{
    printf("[%d]: ",(int)_startup_time);
    va_list args;
    va_start(args, msg);
    vsprintf(tmpStr,msg, args);
    va_end(args);
    printf("%s\n",tmpStr);
}
void uError(const char *file, int line,const char* msg, ...)
{
    printf("[%d] ERROR: (%s line:%d) ",(int)_startup_time,file,line);
    va_list args;
    va_start(args, msg);
    vsprintf(tmpStr,msg, args);
    va_end(args);
    printf("%s\n",tmpStr);
}
#ifdef __cplusplus
}
#endif
