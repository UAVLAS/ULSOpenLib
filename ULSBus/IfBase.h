#ifndef IFBASE_H
#define IFBASE_H
#include<inttypes.h>
typedef struct
{
    uint8_t*  buf;
    uint32_t  lenght;
}_if_buffer_instance;

class IfBase
{
public:
    IfBase()
    {

    };
    virtual void open(){};
    virtual void task(){};
    virtual uint32_t read(){return 0;};
    virtual bool send(_if_buffer_instance *ifBufferInstace){(void)ifBufferInstace;return false;};
    virtual uint32_t maxTxPacketLenght(){return 0;}

    bool send(uint8_t *buf, uint32_t lenght)
    {
        txBufInstance.buf = buf;
        txBufInstance.lenght = lenght;
        return send(&txBufInstance);
    };
    bool send()
    {
      return send(&txBufInstance);
    }
    _if_buffer_instance rxBufInstance;
    _if_buffer_instance txBufInstance;
    virtual void enableEscIfSupprted(bool esc)
    {
        (void)esc;
        //enableEsc(esc);
    }
protected:
};



#endif // IFBASE_H
