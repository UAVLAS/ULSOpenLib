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

#include "ULSBusObject.h"

ULSBusObjectBase::ULSBusObjectBase(uint16_t id,
                                   const char* pxDescription,
                                   _ulsbus_obj_permitions permition,
                                   uint16_t size,
                                   uint8_t *pxData):
    ULSListItem(),
    _id(id),
    _size(size),
    _pxData(pxData),
    _pxDescription(pxDescription),
    _permition(permition)
{

};
void ULSBusObjectBase::lock()
{
    OS_ENTER_CRITICAL; // protect for slow operations (Ex. array copy)
};
void ULSBusObjectBase::unlock()
{
    OS_EXIT_CRITICAL;
};
void ULSBusObjectBase::getData(uint8_t *buf)
{
    lock();
    memcpy(buf,_pxData,_size);
    unlock();
}
void ULSBusObjectBase::setData(uint8_t *buf)
{
    lock();
    memcpy(_pxData,buf,_size);
    unlock();
    // TODO: _updated.emmit(this);
}
const char* ULSBusObjectBase::description()
{
    return _pxDescription;
}
uint16_t ULSBusObjectBase::id()
{
    return _id;
}
_ulsbus_obj_permitions ULSBusObjectBase::permition()
{
    return _permition;
}
uint16_t ULSBusObjectBase::size()
{
    return _size;
};
uint8_t* ULSBusObjectBase::data()
{
    return _pxData;
};

ULSBusObjectsDictionary::ULSBusObjectsDictionary():
    ULSList(),
    ULSListItem()
{

};

ULSBusObjectBase* ULSBusObjectsDictionary::getObject(uint16_t id)
{
    ULSBusObjectBase *px = head();
    while(px){
        if(px->id()==id)return px;
        px = forward(px);
    };
    return __null;
}
_ulsbus_obj_find_rezult ULSBusObjectsDictionary::find(uint16_t obj_id,uint16_t size)
{
    ULSBusObjectBase *px = head();
    while(px){
        if(px->id()==obj_id){
            if(px->size() == size){
                return ULSBUS_OBJECT_FIND_OK;
            }else{
                while(1);
                return ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH;
            }
        }
        px = forward(px);
    };
    return ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND;
}






