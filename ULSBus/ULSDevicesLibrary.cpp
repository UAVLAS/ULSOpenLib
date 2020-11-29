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
ULSDeviceBase* ULSDevicesLibrary::findDevice(uint8_t id)
{
    ULSDeviceBase *px = head();
    while(px){
        if(px->connected())
        {
            if(px->id() == id){
                return px;
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
