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



ULSDevice_ULSX::ULSDevice_ULSX(const char* name,uint8_t id,uint16_t devClass,uint16_t hardware):
ULSDeviceBase(name,id,devClass,hardware),
  o_signature(this,0x0001,"Device/Signature","Device signature data",ULSBUS_OBJECT_PERMITION_SYSCONFIG),
  o_sys_cmd(this,0x0010,"Device/Command","Device command",ULSBUS_OBJECT_PERMITION_WRITEONLY),
  o_sys_status(this,0x0011,"Device/Status","Device status data",ULSBUS_OBJECT_PERMITION_READONLY),
  o_sys_flash(this,0x0100,"Device/Flash","Device flash page",ULSBUS_OBJECT_PERMITION_WRITEONLY)
{
    add(&o_signature);
    add(&o_sys_cmd);
    add(&o_sys_status);
    add(&o_sys_flash);

}
