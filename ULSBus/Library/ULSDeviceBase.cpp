#include "ULSDeviceBase.h"


ULSDeviceBase::ULSDeviceBase(const char* name,uint8_t id,uint16_t devClass,uint16_t hardware):
    ULSList(),
    ULSListItem(),
  _connected(false)
{
    _status.id = id;
    _status.devClass = devClass;
    _status.hardware = hardware;
    _name = name;
}
void ULSDeviceBase::connected(bool connected)
{
    _connected = connected;
}
bool ULSDeviceBase::connected()
{
    return _connected;
}
uint8_t  ULSDeviceBase::id()
{
    return _status.id;
}
void   ULSDeviceBase::id(uint8_t id)
{
    _status.id = id;
}
void ULSDeviceBase::status(_ulsbus_device_status* status)
{
    _status = *status;
}
_ulsbus_device_status* ULSDeviceBase::status()
{
   return  &_status;
}
ULSBusObjectBase* ULSDeviceBase::getObject(uint16_t id)
{
    ULSBusObjectBase *px = head();
    while(px){
        if(px->id()==id)return px;
        px = forward(px);
    };
    return __null;
}
void ULSDeviceBase::updatedCallback(_ulsbus_obj_updated_callback callback)
{
    ULSBusObjectBase *px = head();
    while(px){
        px->updatedCallback(callback);
        px = forward(px);
    };
}

