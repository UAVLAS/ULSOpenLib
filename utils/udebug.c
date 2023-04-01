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
void udebugElspsed(unsigned int etaime)
{
    _startup_time = etaime;
}
void uDebug(const char* msg, ...)
{
   char* pxOutBuf = tmpStr;
   pxOutBuf += sprintf(pxOutBuf,"[%d]: ",(int)_startup_time);
    va_list args;
    va_start(args, msg);
    vsprintf(pxOutBuf,msg, args);
    va_end(args);
    ULS_DBUG_MSG(tmpStr);
}
void uDebugPacket(const char* msg,const char* msg2, uint8_t *buf,uint32_t len)
{
    char* pxOutBuf = tmpStr;
    pxOutBuf += sprintf(pxOutBuf,"[%d]: %s:%s Packet[%d]:",(int)_startup_time,msg,msg2,(int)len);
    while(len--){
        pxOutBuf += sprintf(pxOutBuf,"0x%.2X ",*buf++);
    }
    ULS_DBUG_MSG(tmpStr);
}
void uError(const char *file, int line,const char* msg, ...)
{
    char* pxOutBuf = tmpStr;
    pxOutBuf += sprintf(pxOutBuf,"[%d] ERROR: (%s line:%d) ",(int)_startup_time,file,line);
    va_list args;
    va_start(args, msg);
    vsprintf(pxOutBuf,msg, args);
    va_end(args);
    ULS_DBUG_MSG(tmpStr);
}
#ifdef __cplusplus
}
#endif
