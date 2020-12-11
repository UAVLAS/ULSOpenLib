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

#include "ULSBusQTWrapper.h"

ULSBusQTWrapper::ULSBusQTWrapper(const char* name,ULSDeviceBase *selfDevice,_ulsbus_obj_updated_callback callback):
    ULSBus(name,selfDevice),
    _callback(callback)
{
    add(_transactions,512);
    add(_objectsBuffers,512);
}

void ULSBusQTWrapper::addDevice(_ulsbus_device_status *status){
    ULSDeviceBase *dev;
    if((status->hardware&0x8000)==0){
        switch(status->devClass)
        {
        case __DEVICE_CLASS_ULSQR1:
            if(status->hardware == __DEVICE_HW_ULSQR1_R1)
            {
                dev = new  ULSDevice_ULSQR1_R1(status->id);
            }else{
                return;
            }
            break;
        case __DEVICE_CLASS_ULSQT1:
            if(status->hardware == __DEVICE_HW_ULSQT1_R1)
            {
                dev = new  ULSDevice_ULSQT1_R1(status->id);
            }else{
                return;
            }

            break;
        default:
            dev = new  ULSDevice_ULSX("ULSX",status->id,status->devClass,status->hardware);
            break;
        }
    }else{
        dev = new  ULSDevice_ULSX_LDR("ULSX_LDR",status->id,status->devClass,status->hardware);
    }

    if(!dev)return; //WTF?
    dev->connected(true);
    dev->updatedCallback(_callback);
    _remoteDevices.add(dev);
}
