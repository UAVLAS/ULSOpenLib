#ifndef ULSDEVICESLIBRARY_H
#define ULSDEVICESLIBRARY_H
#include "ULSDeviceBase.h"

class ULSDevicesLibrary:public ULSList<ULSDeviceBase>
{
public:
    ULSDevicesLibrary();
    ULSBusObjectBase* getObject(uint8_t self_id,uint8_t remote_id,uint16_t obj_id);
    _ulsbus_obj_find_rezult find(uint8_t self_id,uint8_t remote_id,uint16_t obj_id,uint16_t size);
    _ulsbus_device_status *findDevices(uint8_t self_id,uint8_t remote_id);

};
#endif // ULSDEVICESLIBRARY_H
