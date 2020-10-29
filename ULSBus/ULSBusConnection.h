#ifndef ULSBUSCONNECTION_H
#define ULSBUSCONNECTION_H
#include "ULSBusTypes.h"

class ULSBusConnection:public ULSListItem
{
public:
    ULSBusConnection(IfBase* interface = __null,uint16_t maxFarameSize = 0):ULSListItem(),
        _interface(interface),
        _maxFarameSize(maxFarameSize)
    {

    };
    void refresh(uint8_t id)
    {
        _timeout[id] = ULSBUS_NM_TIMEOUT;
    };
    void task(){
        uint32_t *pxTimeOut = _timeout;
        uint32_t i = 255;
        while(i > 0U)
        {
            if(*pxTimeOut>0U){(*pxTimeOut)--;}
            pxTimeOut++;
            i--;
        }
        if(_interface)_interface->task();
    }
    uint32_t read(){return _interface->read();};
    bool deviceConnected(uint8_t id){
        if(_timeout[id])return true;
        return false;
    }
    void send(_if_buffer_instance* bufi)
    {
        if(_interface)_interface->send(bufi);
    }
    void send()
    {
        if(_interface)_interface->send();
    }
    bool sendAck(_ulsbus_ack ackcmd,uint8_t self_id,uint8_t remote_id)
    {
        _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_interface->txBufInstance.buf);
        pxPack->ack.cmd = ULSBUS_ACK;
        pxPack->ack.self_id = self_id;
        pxPack->ack.remote_id = remote_id;
        pxPack->ack.ackcmd = ackcmd;
        _interface->txBufInstance.lenght = ULSBUS_HEADER_SIZE_ACK;
        if(_interface){

            _interface->send();
        }
        return true;
    }
    bool sendNM(_ulsbus_device_status *dev)
    {
        if(!_interface)return false;
        _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_interface->txBufInstance.buf);
        pxPack->nm.cmd = ULSBUS_NM;
        pxPack->nm.self_id = dev->self_id;
        pxPack->nm.dev_class = dev->dev_class;
        pxPack->nm.hardware = dev->hardware;
        pxPack->nm.status1 = dev->status1;
        pxPack->nm.status2 = dev->status2;
        _interface->txBufInstance.lenght = ULSBUS_HEADER_SIZE_NM;
        _interface->send();
        return true;
    }
    IfBase*  interface(){return _interface;};
    void     interface(IfBase* interface){_interface = interface;};
    void     maxFrameSize(uint32_t size){_maxFarameSize = size;};
    uint16_t maxFrameSize(){return _maxFarameSize;};

private:
    IfBase* _interface;
    uint32_t _timeout[256];
    uint16_t _maxFarameSize;
};


class ULSBusConnectionsList:public ULSList<ULSBusConnection>
{
public:
    ULSBusConnectionsList():ULSList(){};

    void redirect(ULSBusConnection* pxConnection) // redirect incoming pachet from interface to other
    {
        ULSBusConnection *px = head();
        while(px){
            if( px != pxConnection){
                px->send(&(pxConnection->interface()->rxBufInstance));
            }
            px = (ULSBusConnection *)px->next();
        };
    };
    void sendNM(_ulsbus_device_status *dev)
    {
        ULSBusConnection *px = head();
        while(px){
            px->sendNM(dev);
            px = (ULSBusConnection *)px->next();
        };
    };
    void task()
    {
        ULSBusConnection *px = head();
        while(px){
            px->task();
            px = (ULSBusConnection *)px->next();
        };
    };
    void refresh(ULSBusConnection* pxConnection,uint8_t id)
    {
        ULSBusConnection *px = head();
        while(px){
            if(px == pxConnection) px->refresh(id);
            px = (ULSBusConnection *)px->next();
        };
    };

    ULSBusConnection* findId(uint8_t id)
    {
        ULSBusConnection *px = head();
        while(px){
            if(px->deviceConnected(id)) return px;
            px = (ULSBusConnection *)px->next();
        };
        return __null;
    }

private:

};


#endif // ULSBUSCONNECTION_H
