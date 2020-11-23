#include "ULSDevice_ULSQX.h"

ULSDevice_ULSQR1_R1::ULSDevice_ULSQR1_R1(uint8_t selfId,uint8_t remoteId):
ULSDevice_ULSX("ULSQR1_R1",selfId,remoteId,__DEVICE_CLASS_ULSQR1,__DEVICE_HW_ULSQR1_R1),
  o_status(0x0001,"[Device/Status]Device status data",ULSBUS_OBJECT_PERMITION_SYSCONFIG),
  o_array2000(0x5101,"[Test/Array1]Test array 2000",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2048(0x5102,"[Test/Array2]Test array 2048",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2041(0x5103,"[Test/Array3]Test array 2041",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_tcfgA(0x5001,"[Config/tmp1]Config temp",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_tcfg(0x5002,"[Config/tmp2]Config temp2",ULSBUS_OBJECT_PERMITION_READWRITE)
{
    add(&o_status);
    add(&o_array2000);
    add(&o_array2048);
    add(&o_array2041);

    add(&o_tcfgA);
    add(&o_tcfg);
}
ULSDevice_ULSQT1_R1::ULSDevice_ULSQT1_R1(uint8_t selfId,uint8_t remoteId):
ULSDevice_ULSX("ULSQT1_R1",selfId,remoteId,__DEVICE_CLASS_ULSQT1,__DEVICE_HW_ULSQT1_R1),
  o_status(0x0001,"[Device/Status]Device status data",ULSBUS_OBJECT_PERMITION_SYSCONFIG),
  o_array2000(0x5101,"[Test/Array1]Test array 2000",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2048(0x5102,"[Test/Array2]Test array 2048",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2041(0x5103,"[Test/Array3]Test array 2041",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_tcfgA(0x5001,"[Config/tmp1]Config temp",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_tcfg(0x5002,"[Config/tmp2]Config temp2",ULSBUS_OBJECT_PERMITION_READWRITE)
{
    add(&o_status);
    add(&o_array2000);
    add(&o_array2048);
    add(&o_array2041);

    add(&o_tcfgA);
    add(&o_tcfg);
}
