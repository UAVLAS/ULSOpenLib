#include "ULSDeviceBase.h"


static ULSBusObject<__ulsdb_signature> signature(0x0001,"Device signature data",ULSBUS_OBJECT_PERMITION_SYSCONFIG);
static ULSBusObject<__ulsdbt_sys_cmd>   sys_cmd(0x0010,"Device command",ULSBUS_OBJECT_PERMITION_WRITEONLY);
static ULSBusObject<__ulsdbt_sys_status> sys_status(0x0011,"Device status data",ULSBUS_OBJECT_PERMITION_READONLY);
static ULSBusObject<__ulsdb_sys_flash> sys_flash(0x0100,"Device flash page",ULSBUS_OBJECT_PERMITION_WRITEONLY);


void ULSDeviceBase::init(ULSBusObjectsDictionary *dic)
{
    dic->addObject(&signature);
    dic->addObject(&sys_cmd);
    dic->addObject(&sys_status);
    dic->addObject(&sys_flash);
}
