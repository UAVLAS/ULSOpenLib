#ifndef ULSDEVICE_ULSQX_H
#define ULSDEVICE_ULSQX_H

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

class ULSDevice_ULSQR1_R1:public ULSDevice_ULSX
{
public:
    ULSDevice_ULSQR1_R1(uint8_t selfId,uint8_t remoteId);

    ULSBusObject<__ulsd_ulsqr_status> o_status;
    ULSBusObjectArray<uint8_t,2000> o_array2000;;
    ULSBusObjectArray<uint8_t,2048> o_array2048;
    ULSBusObjectArray<uint8_t,2041> o_array2041;
    ULSBusObjectArray<tmpConfig,2> o_tcfgA;
    ULSBusObject<tmpConfig> o_tcfg;
};
class ULSDevice_ULSQT1_R1:public ULSDevice_ULSX
{
public:
    ULSDevice_ULSQT1_R1(uint8_t selfId,uint8_t remoteId);

    ULSBusObject<__ulsd_ulsqr_status> o_status;
    ULSBusObjectArray<uint8_t,2000> o_array2000;;
    ULSBusObjectArray<uint8_t,2048> o_array2048;
    ULSBusObjectArray<uint8_t,2041> o_array2041;
    ULSBusObjectArray<tmpConfig,2> o_tcfgA;
    ULSBusObject<tmpConfig> o_tcfg;
};
#endif // ULSDEVICE_ULSQX_H
