#ifndef ULSDEVICEBASE_H
#define ULSDEVICEBASE_H
#include "ULSBusObject.h"


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

class ULSDeviceBase
{
public:
    ULSDeviceBase(){};
    void init(ULSBusObjectsDictionary *dic);
};


#endif // ULSDEVICEBASE_H
