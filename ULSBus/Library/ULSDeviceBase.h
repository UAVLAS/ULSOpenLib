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
#ifndef ULSDEVICEBASE_H
#define ULSDEVICEBASE_H
#include "ULSBusObject.h"

#define __DEVICE_CLASS_X     (0x0000)
#define __DEVICE_HW_X        (0x0000)

#define __DEVICE_CLASS_ULSQR1 (0x0010)
#define __DEVICE_HW_ULSQR1_R1 (0x0001)

#define __DEVICE_CLASS_ULSQT1 (0x0020)
#define __DEVICE_HW_ULSQT1_R1 (0x0001)


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

class ULSDeviceBase:public ULSList<ULSBusObjectBase>, public ULSListItem
{
public:
    ULSDeviceBase(const char* name,uint8_t id,uint16_t devClass,uint16_t hardware);
    void connected(bool connected);
    bool connected();
    uint8_t id();
    void  id(uint8_t id);

    void  status(_ulsbus_device_status *st);
    const char* name(){return _name;}
    _ulsbus_device_status   *status();

    ULSBusObjectBase* getObject(uint16_t id);

    void updatedCallback(_ulsbus_obj_updated_callback callback);

private:
    bool _connected;
    const char *_name;
    _ulsbus_device_status _status;
};






#endif // ULSDEVICEBASE_H
