/**
 *  Copyright: 2020 by UAVLAS  <www.uavlas.com>
 *  Author: Yury Kapacheuski <yk@uavlas.com>
 *
 * This file is part of UAVLAS project applications.
 *
 * This is free software: you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Some open source application is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license LGPL-3.0+ <https://spdx.org/licenses/LGPL-3.0+>
 */

#include "ULSDevice_ULSQX.h"

ULSDevice_ULSX::ULSDevice_ULSX(const char* name,uint8_t id,uint16_t devClass,uint16_t hardware):
ULSDeviceBase(name,id,devClass,hardware),
  o_sys_signature(this,0x0001,"System_Signature","Device signature data",ULSBUS_OBJECT_PERMITION_READONLY),
  o_sys_status   (this,0x0002,"System_Status"   ,"Device status data"   ,ULSBUS_OBJECT_PERMITION_READONLY),
  o_sys_cmd      (this,0x0010,"System_Command"  ,"Device command"       ,ULSBUS_OBJECT_PERMITION_WRITEONLY)
{
    add(&o_sys_signature);
    add(&o_sys_cmd);
    add(&o_sys_status);
}

ULSDevice_ULSX_LDR::ULSDevice_ULSX_LDR(const char* name,uint8_t id,uint16_t devClass,uint16_t hardware):
ULSDevice_ULSX(name,id,devClass,hardware),
  o_sys_flash    (this,0x0100,"System_Flash"    ,"Device flash page"    ,ULSBUS_OBJECT_PERMITION_WRITEONLY)
{
    add(&o_sys_flash);
}


ULSDevice_ULSQR1_R1::ULSDevice_ULSQR1_R1(uint8_t id):
ULSDevice_ULSX("ULSQR1_R1",id,__DEVICE_CLASS_ULSQR1,__DEVICE_HW_ULSQR1_R1),
  o_array2000(this,0x5101,"Test_Array1"     ,"Test array 2000"      ,ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2048(this,0x5102,"Test_Array2"     ,"Test array 2048"      ,ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2041(this,0x5103,"Test_Array3"     ,"Test array 2041"      ,ULSBUS_OBJECT_PERMITION_READWRITE)
{
    add(&o_array2000);
    add(&o_array2048);
    add(&o_array2041);
}
ULSDevice_ULSQT1_R1::ULSDevice_ULSQT1_R1(uint8_t id):
ULSDevice_ULSX("ULSQT1_R1",id,__DEVICE_CLASS_ULSQT1,__DEVICE_HW_ULSQT1_R1),
  o_array2000(this,0x5101,"Test_Array1"     ,"Test array 2000"      ,ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2048(this,0x5102,"Test_Array2"     ,"Test array 2048"      ,ULSBUS_OBJECT_PERMITION_READWRITE),
  o_array2041(this,0x5103,"Test_Array3"     ,"Test array 2041"      ,ULSBUS_OBJECT_PERMITION_READWRITE)
{
    add(&o_array2000);
    add(&o_array2048);
    add(&o_array2041);
}


ULSDevice_ULSPC::ULSDevice_ULSPC(uint8_t id):
ULSDevice_ULSX("ULSQT1_R1",id,__DEVICE_CLASS_ULSQT1,__DEVICE_HW_ULSQT1_R1)
{

}
