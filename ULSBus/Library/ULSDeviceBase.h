#ifndef ULSDEVICEBASE_H
#define ULSDEVICEBASE_H
#include "ULSBusObject.h"

#define __DEVICE_CLASS_ULSQR1 0x0010
#define __DEVICE_HW_ULSQR1_R1 0x0001

#define __DEVICE_CLASS_ULSQT1 0x0020
#define __DEVICE_HW_ULSQT1_R1 0x0001


typedef struct __attribute__((packed)){
    char hw[32];
    char fw[32];
    char ldr[32];
    char serial[16];
    uint32_t progflashingtime;
    uint32_t progsize;
    uint32_t progcrc;
    uint32_t devclass;
}__ulsdb_signature;  // Total 128 bytes;



typedef enum  : uint16_t{
    ULSBD_SYS_STARTLOADER = 1,
    ULSBD_SYS_STARTUSER = 2,
    ULSBD_SYS_PREPARE_TO_UPDATE = 10,
    ULSBD_SYS_FINISH_UPDATE = 11

}__ulsdbt_sys_cmd;


typedef enum : uint16_t{
    ULSBD_SYS_ONUSER = 1,
    ULSBD_SYS_LOADER = 2,
    ULSBD_SYS_READY_TO_UPDATE = 10,
    ULSBD_SYS_UPDATE_DONE = 11,
}__ulsdbt_sys_status;

typedef struct __attribute__((packed)){
    uint32_t addr;
    uint32_t size;
    uint8_t  buf[128];
}__ulsdb_sys_flash;

class ULSDeviceBase: public ULSBusObjectsDictionary
{
public:
    ULSDeviceBase(uint8_t selfId,uint8_t remoteId,uint16_t devClass,uint16_t hardware);

    ULSBusObject<__ulsdb_signature> signature;
    ULSBusObject<__ulsdbt_sys_cmd>   sys_cmd;
    ULSBusObject<__ulsdbt_sys_status> sys_status;
    ULSBusObject<__ulsdb_sys_flash> sys_flash;
};


#endif // ULSDEVICEBASE_H
