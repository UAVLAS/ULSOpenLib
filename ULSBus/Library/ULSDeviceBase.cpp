#include "ULSDeviceBase.h"


ULSDeviceBase::ULSDeviceBase(const char* name,uint8_t selfId,uint8_t remoteId,uint16_t devClass,uint16_t hardware):
ULSBusObjectsDictionary(),
  _connected(false)
{
    _status.self_id = selfId;
    _status.remote_id = remoteId;
    _status.dev_class = devClass;
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
uint8_t  ULSDeviceBase::self_id()
{
    return _status.self_id;
}
uint8_t  ULSDeviceBase::remote_id()
{
    return _status.remote_id;
}
void   ULSDeviceBase::self_id(uint8_t id)
{
    _status.self_id = id;
}
void   ULSDeviceBase::remote_id(uint8_t id)
{
    _status.remote_id = id;
}
_ulsbus_device_status* ULSDeviceBase::status()
{
    return &_status;
}

ULSDevice_ULSX::ULSDevice_ULSX(const char* name,uint8_t selfId,uint8_t remoteId,uint16_t devClass,uint16_t hardware):
ULSDeviceBase(name,selfId,remoteId,devClass,hardware),
  o_signature(0x0001,"[Device/Signature]Device signature data",ULSBUS_OBJECT_PERMITION_SYSCONFIG),
  o_sys_cmd(0x0010,"[Device/Command]Device command",ULSBUS_OBJECT_PERMITION_WRITEONLY),
  o_sys_status(0x0011,"[Device/Status]Device status data",ULSBUS_OBJECT_PERMITION_READONLY),
  o_sys_flash(0x0100,"[Device/Flash]Device flash page",ULSBUS_OBJECT_PERMITION_WRITEONLY)
{
    add(&o_signature);
    add(&o_sys_cmd);
    add(&o_sys_status);
    add(&o_sys_flash);
}
