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
#ifndef ULSDEVICE_ULSQX_H
#define ULSDEVICE_ULSQX_H

#include "ULSDeviceBase.h"

// ULSDevice_ULSX Object types
typedef struct __attribute__((packed)){
    char hw[32];
    char fw[32];
    char ldr[32];
    char serial[16];
    uint32_t progflashingtime;
    uint32_t progsize;
    uint32_t progcrc;
    uint32_t devclass;
}__ulsot_signature;  // Total 128 bytes;

typedef enum : uint16_t{
    ULSOT_SYS_STATUS_ONUSER = 1,
    ULSOT_SYS_STATUS_LOADER = 2,
    ULSOT_SYS_STATUS_READY_TO_UPDATE = 10,
    ULSOT_SYS_USTATUS_PDATE_DONE = 11,
}__ulsot_sys_status;

typedef enum  : uint16_t{
    ULSOT_SYS_CMD_STARTLOADER = 1,
    ULSOT_SYS_CMD_STARTUSER = 2,
    ULSOT_SYS_CMD_PREPARE_TO_UPDATE = 10,
    ULSOT_SYS_CMD_FINISH_UPDATE = 11
}__ulsot_sys_cmd;

typedef struct __attribute__((packed)){
    uint32_t addr;
    uint32_t size;
    uint8_t  buf[128];
}__ulsot_sys_flash;


class ULSDevice_ULSX:public ULSDeviceBase
{
public:
    ULSDevice_ULSX(const char* name,uint8_t id,uint16_t devClass,uint16_t hardware);

    ULSBusObject<__ulsot_signature> o_sys_signature;
    ULSBusObject<__ulsot_sys_status> o_sys_status;
    ULSBusObject<__ulsot_sys_cmd>   o_sys_cmd;
};

class ULSDevice_ULSX_LDR:public ULSDevice_ULSX
{
public:
    ULSDevice_ULSX_LDR(const char* name,uint8_t id,uint16_t devClass,uint16_t hardware);
    ULSBusObject<__ulsot_sys_flash> o_sys_flash;
};

class ULSDevice_ULSQR1_R1:public ULSDevice_ULSX
{
public:
    ULSDevice_ULSQR1_R1(uint8_t id);
    ULSBusObjectArray<uint8_t,2000> o_array2000;;
    ULSBusObjectArray<uint8_t,2048> o_array2048;
    ULSBusObjectArray<uint8_t,2041> o_array2041;
};
class ULSDevice_ULSQT1_R1:public ULSDevice_ULSX
{
public:
    ULSDevice_ULSQT1_R1(uint8_t id);
    ULSBusObjectArray<uint8_t,2000> o_array2000;;
    ULSBusObjectArray<uint8_t,2048> o_array2048;
    ULSBusObjectArray<uint8_t,2041> o_array2041;
};
class ULSDevice_ULSPC:public ULSDevice_ULSX
{
public:
    ULSDevice_ULSPC(uint8_t id);


};
#endif // ULSDEVICE_ULSQX_H
