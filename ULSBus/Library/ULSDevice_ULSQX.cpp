#include "ULSDevice_ULSQX.h"

ULSDevice_ULSQR1_R1::ULSDevice_ULSQR1_R1(uint8_t id):
ULSDevice_ULSX("ULSQR1_R1",id,__DEVICE_CLASS_ULSQR1,__DEVICE_HW_ULSQR1_R1),
  o_status(this,0x1001,"Device_Status","Device status data",ULSBUS_OBJECT_PERMITION_SYSCONFIG),
  o_array2000(this,0x5101,"Test_Array1","Test array 2000",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2048(this,0x5102,"Test_Array2","Test array 2048",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2041(this,0x5103,"Test_Array3","Test array 2041",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_tcfgA(this,0x5001,"[Config_tmp1","Config temp",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_tcfg(this,0x5002,"[Config_tmp2","Config temp2",ULSBUS_OBJECT_PERMITION_READWRITE)
{
    add(&o_status);
    add(&o_array2000);
    add(&o_array2048);
    add(&o_array2041);

    add(&o_tcfgA);
    add(&o_tcfg);
}
ULSDevice_ULSQT1_R1::ULSDevice_ULSQT1_R1(uint8_t id):
ULSDevice_ULSX("ULSQT1_R1",id,__DEVICE_CLASS_ULSQT1,__DEVICE_HW_ULSQT1_R1),
  o_status(this,0x1001,"Device_Status","Device status data",ULSBUS_OBJECT_PERMITION_SYSCONFIG),
  o_array2000(this,0x5101,"Test_Array1","Test array 2000",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2048(this,0x5102,"Test_Array2","Test array 2048",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2041(this,0x5103,"Test_Array3","Test array 2041",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_tcfgA(this,0x5001,"Config_tmp1","Config temp",ULSBUS_OBJECT_PERMITION_READWRITE),
  o_tcfg(this,0x5002,"Config_tmp2","Config temp2",ULSBUS_OBJECT_PERMITION_READWRITE)
{
    add(&o_status);
    add(&o_array2000);
    add(&o_array2048);
    add(&o_array2041);

    add(&o_tcfgA);
    add(&o_tcfg);
}


ULSDevice_ULSPC::ULSDevice_ULSPC(uint8_t id):
ULSDevice_ULSX("ULSQT1_R1",id,__DEVICE_CLASS_ULSQT1,__DEVICE_HW_ULSQT1_R1)
{

}
