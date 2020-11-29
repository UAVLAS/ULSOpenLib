#ifndef ULSDEVICESLIBRARY_H
#define ULSDEVICESLIBRARY_H
#include "ULSDeviceBase.h"

class ULSDevicesLibrary:public ULSList<ULSDeviceBase>
{
public:
    ULSDevicesLibrary();
    ULSBusObjectBase* getObject(uint8_t id,uint16_t obj_id);
    ULSDeviceBase *findDevice(uint8_t id);
    void updatedCallback(_ulsbus_obj_updated_callback callback);

};
#endif // ULSDEVICESLIBRARY_H
