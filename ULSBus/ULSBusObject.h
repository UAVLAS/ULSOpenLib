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

#ifndef ULSBUSOBJECT_H
#define ULSBUSOBJECT_H

#include <inttypes.h>
#include <string.h>
#include "OsSupport.h"
#include "ULSBusTypes.h"

class ULSBusObjectBase;
typedef void (*_ulsbus_obj_updated_callback)(void* parent,ULSBusObjectBase*);


template<typename T>
class ObjData
{
public:
    inline T get()
    {
        OS_DISABLE_IRQ; // protect for fast operations
        T tmp = _val;
        OS_ENABLE_IRQ;
        return tmp;
    }
    inline void set(T val)
    {
        OS_DISABLE_IRQ;// protect for fast operations
        _val = val;
        OS_ENABLE_IRQ;
    }
    inline T* val()
    {
        return &_val;
    }
private:
    T _val;
};

class ULSBusObjectBase:public ULSListItem
{
public:
    ULSBusObjectBase(void* parent,
                     uint16_t id,
                     const char* name,
                     const char* description,
                     _ulsbus_obj_permitions permition,
                     uint16_t size,
                     uint16_t len,
                     uint8_t *pxData);

    void getData(uint8_t *buf);
    void setData(uint8_t *buf);
    const char* name();
    const char* description();
    uint16_t id();
    _ulsbus_obj_permitions permition();
    uint16_t size();
    uint16_t len();
    uint8_t* data();

    void lock();
    void unlock();
    void updatedCallback(_ulsbus_obj_updated_callback callback);
    void* parent();

private:
    void* _parent; // pointer to parent object (ULSDevice exspected)
    uint16_t _id;
    uint16_t _size;
    uint16_t _len;
    uint8_t  *_pxData;
    const char *_name;
    const char *_description;
    _ulsbus_obj_permitions _permition;
    _ulsbus_obj_updated_callback _updated_callback;

};

template<typename T,int LENGHT=1>
class ULSBusObjectArray: public ULSBusObjectBase
{
public :
    ULSBusObjectArray(void* parent,uint16_t id,const char* name, const char* decription,_ulsbus_obj_permitions permition):
        ULSBusObjectBase(parent,id,name,decription,permition,LENGHT*sizeof(T) ,LENGHT,(uint8_t*)&var){};
    ObjData<T> var[LENGHT];
};

template<typename T>
class ULSBusObject: public ULSBusObjectBase
{
public :
    ULSBusObject(void* parent,uint16_t id,const char* name, const char* decription,_ulsbus_obj_permitions permition):
        ULSBusObjectBase(parent,id,name,decription,permition,sizeof(T) ,1,(uint8_t*)&var){};
    ObjData<T> var;
};


#endif // ULSBUSOBJECT_H
