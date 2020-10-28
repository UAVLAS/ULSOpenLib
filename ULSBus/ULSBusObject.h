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

class ULSBusObjectBase
{
public:
    ULSBusObjectBase(uint16_t id, const char* pxDescription,_ulsbus_obj_permitions permition,uint16_t size,uint8_t *pxData):
        next(__null),
        _id(id),
        _size(size),
        _pxData(pxData),
        _pxDescription(pxDescription),
        _permition(permition)
    {

    };
    void lock()
    {
        // TODO Object critical section
    };
    void unlock()
    {

    };
    void getData(uint8_t *buf)
    {
        OS_ENTER_CRITICAL; // protect for slow operations (Ex. array copy)
        memcpy(buf,_pxData,_size);
        OS_EXIT_CRITICAL;

    }
    void setData(uint8_t *buf)
    {
        OS_ENTER_CRITICAL; // protect for slow operations (Ex. array copy)
        memcpy(_pxData,buf,_size);
        OS_EXIT_CRITICAL;
       // _updated.emmit(this);
    }
    uint16_t size(){return _size;};
    uint8_t* data(){return _pxData;};

    const char* description(){return _pxDescription;}
    uint16_t id(){return _id;}
    _ulsbus_obj_permitions permition(){return _permition;}
    ULSBusObjectBase* next; // ULSBusObject List support;
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
        ULSBusObjectBase(id,pxDescription,permition,SIZE*sizeof(T) ,(uint8_t*)&data)
    {

    };
    ObjData<T> data[SIZE];
private:

};
template<typename T>
class ULSBusObject: public ULSBusObjectBase
{
public :
    ULSBusObject(uint16_t id, const char* pxDescription,_ulsbus_obj_permitions permition):
        ULSBusObjectBase(id,pxDescription,permition,sizeof(T) ,(uint8_t*)&data)
    {

    };
    ObjData<T> data;
private:

};

class ULSBusObjectsDictionary
{
public:
    ULSBusObjectsDictionary(uint8_t selfId,uint8_t remoteId,uint16_t devClass,uint16_t hardware):
        next(__null),
        _obj_list(__null)
    {
        _devStatus.self_id = selfId;
        _devStatus.remote_id = remoteId;
        _devStatus.dev_class = devClass;
        _devStatus.hardware = hardware;
    };
    void addObject(ULSBusObjectBase *pxObject)
    {
        if(_obj_list == __null){
            _obj_list = pxObject; //add first item;
            return;
        }
        ULSBusObjectBase *pxObj = _obj_list;

        while(pxObj->next != __null){
            pxObj = pxObj->next;
        }
        pxObj->next = pxObject; //add first item;
    }
    ULSBusObjectBase* getObject(uint16_t id)
    {
        ULSBusObjectBase *pxObj = _obj_list;
        while(pxObj != __null){
            if(pxObj->id()==id)return pxObj;
            pxObj = pxObj->next;
        }
        return __null;
    }
    _ulsbus_obj_find_rezult find(uint16_t obj_id,uint16_t size)
    {
        ULSBusObjectBase *pxObj = _obj_list;
        while(pxObj != __null){
            if(pxObj->id()==obj_id){
                if(pxObj->size() == size){

                    return ULSBUS_OBJECT_FIND_OK;
                }else{
                    return ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH;
                }
            }return ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND;
        pxObj = pxObj->next;
        }
        return ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND;
    }
    uint8_t  self_id(){return _devStatus.self_id;}
    uint8_t  remote_id(){return _devStatus.remote_id;}
    void   self_id(uint8_t id){_devStatus.self_id = id;}
    void   remote_id(uint8_t id){_devStatus.remote_id = id;}
    void   devStatus(_ulsbus_device_status *status){_devStatus = *status;}
    _ulsbus_device_status   *devStatus(){return &_devStatus;}

    ULSBusObjectsDictionary* next; //ULSBusObjectsDictionary list support
protected:
    ULSBusObjectBase* _obj_list;
    _ulsbus_device_status _devStatus;
};

class ULSBusObjectsLibrary
{
public:
    ULSBusObjectsLibrary():_od_list(__null)
    {

    };
    void addDictionary(ULSBusObjectsDictionary *pxDictionary)
    {
        if(_od_list == __null){
            _od_list = pxDictionary; //add first item;
        }else{
            ULSBusObjectsDictionary *pxOd = _od_list;
            while(pxOd->next != __null){
                pxOd = pxOd->next;
            }
            pxOd->next = pxDictionary; //add first item;
        }
    }
    ULSBusObjectBase* getObject(uint8_t self_id,uint8_t remote_id,uint16_t obj_id)
    {

        ULSBusObjectsDictionary *pxOd = _od_list;
        while(pxOd != __null){
            if(((pxOd->self_id() == self_id)||(pxOd->self_id() == 0)) &&
                    ((pxOd->remote_id() == remote_id)||(pxOd->remote_id() == 0))){
                return pxOd->getObject(obj_id);
            }
             pxOd = pxOd->next;
        }
        return __null;
    }
    _ulsbus_obj_find_rezult find(uint8_t self_id,uint8_t remote_id,uint16_t obj_id,uint16_t size)
    {
        ULSBusObjectsDictionary *pxOd = _od_list;
        while(pxOd != __null){

            if((pxOd->remote_id() == remote_id) && (pxOd->self_id() == self_id)){
                return pxOd->find(obj_id,size);     
            }
             pxOd = pxOd->next;
        }
        return ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND;
    }
    _ulsbus_device_status *findDevices(uint8_t self_id,uint8_t remote_id)
    {
        ULSBusObjectsDictionary *pxOd = _od_list;
        while(pxOd != __null){

            if((pxOd->remote_id() == remote_id) && (pxOd->self_id() == self_id)){
                return pxOd->devStatus();
            }
             pxOd = pxOd->next;
        }
        return __null;
    }
    ULSBusObjectsDictionary* head(){return _od_list;};
protected:
    ULSBusObjectsDictionary* _od_list;
};

#endif // ULSBUSOBJECT_H
