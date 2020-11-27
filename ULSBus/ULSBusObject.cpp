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

ULSBusObjectBase::ULSBusObjectBase(void* parent,
                                   uint16_t id,
                                   const char* name,
                                   const char* description,
                                   _ulsbus_obj_permitions permition,
                                   uint16_t size,
                                   uint16_t len,
                                   uint8_t *data):
    ULSListItem(),
    _data_state(ULSBUS_OBJECT_DATA_STATE_UNINITIALIZED),
    _parent(parent),
    _id(id),
    _size(size),
    _len(len),
    _data(data),
    _name(name),
    _description(description),
    _permition(permition),
    _state(ULSBUS_OBJECT_STATE_OK),
    _updated_callback(__null)
{

};
void ULSBusObjectBase::lock()
{
    ULS_ENTER_CRITICAL; // protect for slow operations (Ex. array copy)
};
void ULSBusObjectBase::unlock()
{
    ULS_EXIT_CRITICAL;
};
void ULSBusObjectBase::getData(uint8_t *buf)
{
    lock();
    memcpy(buf,_data,_size);
    unlock();
}
void ULSBusObjectBase::setData(uint8_t *buf)
{
    lock();
    memcpy(_data,buf,_size);
    dataState(ULSBUS_OBJECT_DATA_STATE_VALID);
    unlock();
    // TODO: _updated.emmit(this);
}
void ULSBusObjectBase::updatedCallback(_ulsbus_obj_updated_callback callback)
{
    _updated_callback = callback;
}
_ulsbus_obj_state ULSBusObjectBase::state()
{
    return _state;
}
void  ULSBusObjectBase::state(_ulsbus_obj_state state)
{
    _state = state;
    if(_state != ULSBUS_OBJECT_STATE_BUSY){
    if(_updated_callback)_updated_callback(_parent,this);
    }
}
_ulsbus_obj_data_state ULSBusObjectBase::dataState()
{
    return _data_state;
}
void  ULSBusObjectBase::dataState(_ulsbus_obj_data_state state)
{
    _data_state = state;
}

void* ULSBusObjectBase::parent()
{
    return _parent;
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
uint16_t ULSBusObjectBase::len()
{
    return _len;
};
uint8_t* ULSBusObjectBase::data()
{
    return _data;
};
const char* ULSBusObjectBase::name()
{
    return _name;
}
const char* ULSBusObjectBase::description()
{
    return _description;
}






