#ifndef ULSBUSTYPES_H
#define ULSBUSTYPES_H
#include <inttypes.h>
#include <string.h>
#include "ULSDevices.h"
#include "ULSBusConfig.h"
#include "udebug.h"

#define ULSBUS_FRAMESIZE_MAX    256
#define ULSBUS_FRAMES           8
#define ULSBUS_TIMEOUT          50
#define ULSBUS_TIMEOUT_TRANSMIT_COMPLITE    400



#define ULSBUS_TIMEOUT_ACK      200

#define ULSBUS_NM_INTERVAL      1000
#define ULSBUS_NM_TIMEOUT       2000

#define ULSBUS_BUFFERSIZE    (6+256+2) // 6 Header + 256 payload + 2 crc

#define ULSBUS_INTERFACES_MAX  (16)

#define ULSBUS_HEADER_SIZE_NM     (12)
#define ULSBUS_HEADER_SIZE_ACK     (4)



#define ULSBUS_HEADER_SIZE_BOI_SFT     (4)
#define ULSBUS_HEADER_SIZE_BOI_SOT     (10)
#define ULSBUS_HEADER_SIZE_BOI_F       (4)

#define ULSBUS_HEADER_SIZE_RWOI_SFT    (6)
#define ULSBUS_HEADER_SIZE_RWOI_SOT    (12)
#define ULSBUS_HEADER_SIZE_RWOI_F      (4)

#define ULSBUS_HEADER_SIZE_RROI        (8)
#define ULSBUS_HEADER_SIZE_AOI_SFT     (6)
#define ULSBUS_HEADER_SIZE_AOI_SOT     (12)
#define ULSBUS_HEADER_SIZE_AOI_F       (4)


#define ULSBUS_DEVICECLASS_QLSQT1       (10)
#define ULSBUS_DEVICECLASS_QLSQR1       (11)

#define ULSBUS_DEVICEHARDWARE_R0       (0)
#define ULSBUS_DEVICEHARDWARE_R1       (1)


typedef enum{
    ULSBUS_RTSP = 0,
    ULSBUS_SEC = 1,
    ULSBUS_NM = 2,
    ULSBUS_ACK = 8,
    ULSBUS_RLF = 9,
    ULSBUS_BOI_SFT = 14,
    ULSBUS_BOI_SOT = 15,
    ULSBUS_BOI_F = 16,
    ULSBUS_BOI_C = 17,
    ULSBUS_RWOI_SFT = 18,
    ULSBUS_RWOI_SOT = 19,
    ULSBUS_RWOI_F = 20,
    ULSBUS_RWOI_C = 21,
    ULSBUS_RROI = 22,
    ULSBUS_AOI_SFT = 23,
    ULSBUS_AOI_SOT = 24,
    ULSBUS_AOI_F = 25,
    ULSBUS_AOI_C = 26,
}_ulsbus_commands;

typedef enum{
    ULSBUS_ACK_OK = 0,
    ULSBUS_ACK_COMPLITE = 1,
    ULSBUS_ACK_OBJECT_ACCESS_DENIED = 2,
    ULSBUS_ACK_OBJECT_NOTFOUND = 3,
    ULSBUS_ACK_OBJECT_SIZE_MISMUTCH = 4,
    ULSBUS_ACK_OBJECT_CRC_MISMUTCH = 5,
    ULSBUS_ACK_BUFFER_FULL = 6
}_ulsbus_ack;


typedef enum{
    ULSBUST_EMPTY,
    ULSBUST_BUSY,
    ULSBUST_TRANSMIT_COMPLITE_WAIT_ACK,

    ULSBUST_BOI_TRANSMIT_START,
    ULSBUST_BOI_TRANSMIT_START_REPEAT,

    ULSBUST_BOI_TRANSMIT_F,
    ULSBUST_BOI_RECEIVE_START,
    ULSBUST_BOI_RECEIVE_F,



    ULSBUST_RWOI_TRANSMIT_START,
    ULSBUST_RWOI_TRANSMIT_START_REPEAT,

    ULSBUST_RWOI_TRANSMIT_F,


    ULSBUST_RWOI_RECEIVE_START,
    ULSBUST_RWOI_RECEIVE_F,

    ULSBUST_RROI_TRANSMIT_START_REPEAT,

    ULSBUST_AOI_TRANSMIT_START,
    ULSBUST_AOI_TRANSMIT_START_REPEAT,
    ULSBUST_AOI_TRANSMIT_F,

    ULSBUST_AOI_RECEIVE_START,
    ULSBUST_AOI_RECEIVE_F,


    ULSBUS_TP_EOT,
    ULSBUS_TP_COMPLETE
}_ulsbus_transaction_state;

typedef enum{
    ULSBUS_OBJECT_PERMITION_READONLY = 0,
    ULSBUS_OBJECT_PERMITION_WRITEONLY = 1,
    ULSBUS_OBJECT_PERMITION_READWRITE = 2,
    ULSBUS_OBJECT_PERMITION_PROTECTED = 3,
    ULSBUS_OBJECT_PERMITION_CONFIG    = 4,
    ULSBUS_OBJECT_PERMITION_SYSCONFIG = 4,
    ULSBUS_OBJECT_PERMITION_ADMIN     = 4,
}_ulsbus_obj_permitions;

typedef enum{
    ULSBUS_OBJECT_DATA_STATE_UNINITIALIZED = 0, // Object created with no data on it
    ULSBUS_OBJECT_DATA_STATE_VALID = 1,// Data in object is valid
    ULSBUS_OBJECT_DATA_STATE_NOTVALID = 2, // Data in object not valid
}_ulsbus_obj_data_state;

typedef enum{
    ULSBUS_OBJECT_STATE_OK = 0,
    ULSBUS_OBJECT_STATE_BUSY = 1,
    ULSBUS_OBJECT_STATE_ERROR = 2,
    ULSBUS_OBJECT_STATE_TIMEOUT = 3,
}_ulsbus_obj_state;


typedef struct{
    uint8_t  id;
    uint16_t devClass;
    uint16_t hardware;
    uint16_t status1;
    uint16_t status2;
}_ulsbus_device_status;

typedef union{
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint16_t ctrl;
        uint8_t data[8];
    }__attribute__((packed))hdr;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t format;
        uint8_t pulse_id;
        uint64_t time;
        uint64_t time2;
    }__attribute__((packed))rtsp;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t error;
        uint16_t counter;
        uint16_t rezerv[8];
    }__attribute__((packed))sec;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint16_t zeros;
        uint16_t dev_class;
        uint16_t hardware;
        uint16_t status1;
        uint16_t status2;
    }__attribute__((packed))nm;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t ackcmd;
        uint8_t rezerv[8];
    }__attribute__((packed))ack;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t frame_idx;
        uint8_t data[8];
    }__attribute__((packed))rlf;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint16_t obj_id;
        uint8_t data[8];
    }__attribute__((packed))boi_sft;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint16_t obj_id;
        uint8_t frame_last_idx;
        uint8_t byte_last_idx;
        uint16_t crc;
        uint16_t obj_size;
        uint8_t data[2];
    }__attribute__((packed))boi_sot;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t zero;
        uint8_t frame_idx;
        uint8_t data[8];
    }__attribute__((packed))boi_f;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t size;
        uint16_t obj_id;
        uint8_t data[6];
    }__attribute__((packed))rwoi_sft;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t zero;
        uint16_t obj_id;
        uint8_t frame_last_idx;
        uint8_t byte_last_idx;
        uint16_t crc;
        uint16_t obj_size;
    }__attribute__((packed))rwoi_sot;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t frame_idx;
        uint8_t data[8];
    }__attribute__((packed))rwoi_f;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t zero;
        uint16_t obj_id;
        uint16_t size;
        uint8_t data[4];
    }__attribute__((packed))rroi;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t size;
        uint16_t obj_id;
        uint8_t data[6];
    }__attribute__((packed))aoi_sft;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t zero;
        uint16_t obj_id;
        uint8_t frame_last_idx;
        uint8_t byte_last_idx;
        uint16_t crc;
        uint16_t size;
    }__attribute__((packed))aoi_sot;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t frame_idx;
        uint8_t data[8];
    }__attribute__((packed))aoi_f;

}__attribute__((packed))_ulsbus_packet;


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
    //    bool find(T* item)
    //    {
    //        begin();
    //        while(next()){
    //            if(current == item){return true;}
    //        }
    //        return false;
    //    };
    T *current;
private:
    bool _first;
    T *_head;
};



#endif // ULSBUSTYPES_H

