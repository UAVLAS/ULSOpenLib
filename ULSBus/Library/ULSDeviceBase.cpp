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

#include "ULSDeviceBase.h"


ULSDeviceBase::ULSDeviceBase(const char* name,uint8_t id,uint16_t devClass,uint16_t hardware):
    ULSList(),
    ULSListItem(),
  _connected(false)
{
    _status.id = id;
    _status.devClass = devClass;
    _status.hardware = hardware;
    _name = name;
}
void ULSDeviceBase::connected(bool connected)
{
    _connected = connected;
}
bool ULSDeviceBase::connected()
{
    return _connected;
}
uint8_t  ULSDeviceBase::id()
{
    return _status.id;
}
void   ULSDeviceBase::id(uint8_t id)
{
    _status.id = id;
}
void ULSDeviceBase::status(_ulsbus_device_status* status)
{
    _status = *status;
}
_ulsbus_device_status* ULSDeviceBase::status()
{
   return  &_status;
}
ULSBusObjectBase* ULSDeviceBase::getObject(uint16_t id)
{
    ULSBusObjectBase *px = head();
    while(px){
        if(px->id()==id)return px;
        px = forward(px);
    };
    return __null;
}
void ULSDeviceBase::updatedCallback(_ulsbus_obj_updated_callback callback)
{
    ULSBusObjectBase *px = head();
    while(px){
        px->updatedCallback(callback);
        px = forward(px);
    };
}

