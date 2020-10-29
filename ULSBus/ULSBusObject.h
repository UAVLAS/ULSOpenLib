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
private:
    T _val;
};

class ULSBusObjectBase:public ULSListItem
{
public:
    ULSBusObjectBase(uint16_t id, const char* pxDescription,_ulsbus_obj_permitions permition,uint16_t size,uint8_t *pxData);

    void getData(uint8_t *buf);
    void setData(uint8_t *buf);
    const char* description();
    uint16_t id();
    _ulsbus_obj_permitions permition();
    uint16_t size();
    uint8_t* data();

private:
    void lock();
    void unlock();

private:
    uint16_t _id;
    uint16_t _size;
    uint8_t  *_pxData;
    const char *_pxDescription;
    _ulsbus_obj_permitions _permition;
};

template<typename T,int SIZE=1>
class ULSBusObjectArray: public ULSBusObjectBase
{
public :
    ULSBusObjectArray(uint16_t id, const char* pxDescription,_ulsbus_obj_permitions permition):
        ULSBusObjectBase(id,pxDescription,permition,SIZE*sizeof(T) ,(uint8_t*)&data){};
    ObjData<T> data[SIZE];
};

template<typename T>
class ULSBusObject: public ULSBusObjectBase
{
public :
    ULSBusObject(uint16_t id, const char* pxDescription,_ulsbus_obj_permitions permition):
        ULSBusObjectBase(id,pxDescription,permition,sizeof(T) ,(uint8_t*)&data){};
    ObjData<T> data;
};

class ULSBusObjectsDictionary:public ULSList<ULSBusObjectBase>, public ULSListItem
{
public:
    ULSBusObjectsDictionary(uint8_t selfId,uint8_t remoteId,uint16_t devClass,uint16_t hardware);
    ULSBusObjectBase* getObject(uint16_t id);
    _ulsbus_obj_find_rezult find(uint16_t obj_id,uint16_t size);
    uint8_t self_id();
    uint8_t remote_id();
    void  self_id(uint8_t id);
    void  remote_id(uint8_t id);
    void  devStatus(_ulsbus_device_status *status);
    _ulsbus_device_status   *devStatus();

protected:
    _ulsbus_device_status _devStatus;
};

class ULSBusObjectsLibrary:public ULSList<ULSBusObjectsDictionary>
{
public:
    ULSBusObjectsLibrary();
    ULSBusObjectBase* getObject(uint8_t self_id,uint8_t remote_id,uint16_t obj_id);
    _ulsbus_obj_find_rezult find(uint8_t self_id,uint8_t remote_id,uint16_t obj_id,uint16_t size);
    _ulsbus_device_status *findDevices(uint8_t self_id,uint8_t remote_id);
};

#endif // ULSBUSOBJECT_H
