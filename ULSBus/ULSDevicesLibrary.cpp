#include "ULSDevicesLibrary.h"


ULSDevicesLibrary::ULSDevicesLibrary():ULSList()
{

};
ULSBusObjectBase* ULSDevicesLibrary::getObject(uint8_t id,uint16_t obj_id)
{
    ULSDeviceBase *px = head();
    while(px){
        if(px->connected())
        {
            if(px->id() == id){
                return px->getObject(obj_id);
            }
        }
        px = forward(px);
    };
    return __null;
}
_ulsbus_obj_find_rezult ULSDevicesLibrary::find(uint8_t id,uint16_t obj_id,uint16_t size)
{
    ULSDeviceBase *px = head();
    while(px){
        if(px->connected())
        {
            if(px->id() == id){
                return px->find(obj_id,size);
            }
        }
        px = forward(px);
    };
    return ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND;
}
_ulsbus_device_status* ULSDevicesLibrary::findDevices(uint8_t id)
{
    ULSDeviceBase *px = head();
    while(px){
        if(px->connected())
        {
            if(px->id() == id){
                return px->status();
            }
        }
        px = forward(px);
    };
    return __null;
}
void ULSDevicesLibrary::updatedCallback(_ulsbus_obj_updated_callback callback)
{
    ULSDeviceBase *px = head();
    while(px){
        px->updatedCallback(callback);
        px = forward(px);
    };
}
