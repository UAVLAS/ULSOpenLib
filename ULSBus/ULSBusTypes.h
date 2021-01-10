#ifndef ULSBUSTYPES_H
#define ULSBUSTYPES_H
#include <inttypes.h>
#include <string.h>
#include "ULSDevices.h"
#include "ULSBusConfig.h"
#include "udebug.h"

#define _ULS_OPERATION_KEY 0x24041982

typedef enum{
    ULSBUS_OBJECT_PERMITION_READONLY = 0,
    ULSBUS_OBJECT_PERMITION_WRITEONLY = 1,
    ULSBUS_OBJECT_PERMITION_READWRITE = 2,
    ULSBUS_OBJECT_PERMITION_PROTECTED = 3,
    ULSBUS_OBJECT_PERMITION_CONFIG    = 4,
    ULSBUS_OBJECT_PERMITION_SYSCONFIG = 5,
    ULSBUS_OBJECT_PERMITION_ADMIN     = 6
}_ulsbus_obj_permitions;

class ULSListItem{
public:
    ULSListItem(){
        pxnext = __null;
        };
    void* pxnext;
};

template<class T>
class ULSList{
public:
    ULSList():_head(nullptr){};
    void begin(){_first = true;}
    bool next()
    {
        if(_first){
            current = _head;
            _first = false;
        }else{
            if(current == nullptr) return false;
            current = (T*)current->pxnext;
        }
        if(current == nullptr) return false;
        return true;
    }
    void add(T* item){
        if(_head == nullptr){
            _head = item; //add first item;
            //_head->pxnext = nullptr;
        }else{
            T *px = (T *)_head;
            while(px->pxnext != nullptr){
                px = (T*)(px->pxnext);
            }
            px->pxnext = item; //add item;
           // item->pxnext = nullptr;
        }
    };
    void remove(T* item){
        // Todo Add remove code
        (void)item;
    };
    T *current;
private:
    bool _first;
    T *_head;
};



#endif // ULSBUSTYPES_H

