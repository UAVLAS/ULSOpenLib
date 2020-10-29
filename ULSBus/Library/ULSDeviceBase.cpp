#include "ULSDeviceBase.h"


ULSDeviceBase::ULSDeviceBase(uint8_t selfId,uint8_t remoteId,uint16_t devClass,uint16_t hardware):
ULSBusObjectsDictionary(selfId,remoteId,devClass,hardware),
  signature(0x0001,"[Device/Signature]Device signature data",ULSBUS_OBJECT_PERMITION_SYSCONFIG),
  sys_cmd(0x0010,"[Device/Command]Device command",ULSBUS_OBJECT_PERMITION_WRITEONLY),
  sys_status(0x0011,"[Device/Status]Device status data",ULSBUS_OBJECT_PERMITION_READONLY),
  sys_flash(0x0100,"[Device/Flash]Device flash page",ULSBUS_OBJECT_PERMITION_WRITEONLY)
{
    add(&signature);
    add(&sys_cmd);
    add(&sys_status);
    add(&sys_flash);
}
