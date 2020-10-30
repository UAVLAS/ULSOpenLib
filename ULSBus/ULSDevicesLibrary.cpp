#include "ULSDevicesLibrary.h"


ULSDevicesLibrary::ULSDevicesLibrary():ULSList()
{

};
ULSBusObjectBase* ULSDevicesLibrary::getObject(uint8_t self_id,uint8_t remote_id,uint16_t obj_id)
{
    ULSDeviceBase *px = head();
    while(px){
        if(px->connected())
        {
            if(((px->self_id() == self_id)||(self_id == 0)) &&
                    ((px->remote_id() == remote_id)||(remote_id == 0))){
                return px->getObject(obj_id);
            }
        }
        px = forward(px);
    };
    return __null;
}
_ulsbus_obj_find_rezult ULSDevicesLibrary::find(uint8_t self_id,uint8_t remote_id,uint16_t obj_id,uint16_t size)
{
    ULSDeviceBase *px = head();
    while(px){
        if(px->connected())
        {
            if(((px->self_id() == self_id)||(self_id == 0)) &&
                    ((px->remote_id() == remote_id)||(remote_id == 0))){
                return px->find(obj_id,size);
            }
        }
        px = forward(px);
    };
    return ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND;
}
_ulsbus_device_status* ULSDevicesLibrary::findDevices(uint8_t self_id,uint8_t remote_id)
{
    ULSDeviceBase *px = head();
    while(px){
        if(px->connected())
        {
            if(((px->self_id() == self_id)||(self_id == 0)) &&
                    ((px->remote_id() == remote_id)||(remote_id == 0))){
                return px->status();
            }
        }
        px = forward(px);
    };
    return __null;
}
