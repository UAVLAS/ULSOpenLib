#include "ULSDevice_ULSQR1.h"

ULSDevice_ULSQR1_R1::ULSDevice_ULSQR1_R1(uint8_t selfId,uint8_t remoteId):
ULSDeviceBase(selfId,remoteId,__DEVICE_CLASS_ULSQR1,__DEVICE_HW_ULSQR1_R1),
  status(0x0001,"Device signature data",ULSBUS_OBJECT_PERMITION_SYSCONFIG),
  array2000(0x5101,"Test array 2000",ULSBUS_OBJECT_PERMITION_READWRITE),
  array2048(0x5102,"Test array 2048",ULSBUS_OBJECT_PERMITION_READWRITE),
  tcfgA(0x5001,"Description 1",ULSBUS_OBJECT_PERMITION_READWRITE),
  tcfg(0x5002,"Description 2",ULSBUS_OBJECT_PERMITION_READWRITE)
{
    add(&status);
    add(&array2000);
    add(&array2048);
    add(&tcfgA);
    add(&tcfg);
}
