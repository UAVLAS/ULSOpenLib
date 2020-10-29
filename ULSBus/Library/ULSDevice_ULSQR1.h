#ifndef ULSDEVICE_ULSQR1_H
#define ULSDEVICE_ULSQR1_H

#include "ULSDeviceBase.h"

typedef struct __attribute__((packed)){
    uint8_t id;
    uint8_t cfga;
    uint8_t cfgb;
    uint8_t cfgc;
    uint32_t data;

}tmpConfig;

typedef struct __attribute__((packed)){
    char hw[32];
    char fw[32];
    char ldr[32];
    char serial[16];
    uint32_t progflashingtime;
    uint32_t progsize;
    uint32_t progcrc;
    uint32_t devclass;
}__ulsd_ulsqr_status;  // Total 128 bytes;

class ULSDevice_ULSQR1_R1:public ULSDeviceBase
{
public:
    ULSDevice_ULSQR1_R1(uint8_t selfId,uint8_t remoteId);

    ULSBusObject<__ulsd_ulsqr_status> status;
    ULSBusObjectArray<uint8_t,2000> array2000;;
    ULSBusObjectArray<uint8_t,2048> array2048;
    ULSBusObjectArray<tmpConfig,2> tcfgA;
    ULSBusObject<tmpConfig> tcfg;
};

#endif // ULSDEVICE_ULSQR1_H
