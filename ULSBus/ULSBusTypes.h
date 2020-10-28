#ifndef ULSBUSTYPES_H
#define ULSBUSTYPES_H
#include <inttypes.h>
#include "IfBase.h"



#define ULSBUS_FRAMESIZE_MAX    256
#define ULSBUS_FRAMES           8
#define ULSBUS_TIMEOUT          50
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
    ULSBUS_ACK_RWOI_SFT_OK = ULSBUS_RWOI_SFT|(0<<5),
   // ULSBUS_ACK_RWOI_SFT_OK_COMPLITE = ULSBUS_RWOI_SFT|(1<<5),   // ULSBUS_ACK_RWOI_SFT_OK - confirms reception
    ULSBUS_ACK_RWOI_SFT_OBJECT_NOTFOUND = ULSBUS_RWOI_SFT|(2<<5),
    ULSBUS_ACK_RWOI_SFT_OBJECT_SIZE_MISNUCH = ULSBUS_RWOI_SFT|(3<<5),
    ULSBUS_ACK_RWOI_SFT_OBJECT_BUFFER_FULL = ULSBUS_RWOI_SFT|(4<<5),
    ULSBUS_ACK_RWOI_SFT_OBJECT_CRC_MISNUCH = ULSBUS_RWOI_SFT|(5<<5),



    ULSBUS_ACK_RWOI_SOT_OK = ULSBUS_RWOI_SOT|(0<<5),
    ULSBUS_ACK_RWOI_SOT_COMPLITE = ULSBUS_RWOI_SOT|(1<<5),
    ULSBUS_ACK_RWOI_SOT_OBJECT_NOTFOUND = ULSBUS_RWOI_SOT|(2<<5),
    ULSBUS_ACK_RWOI_SOT_OBJECT_SIZE_MISMUCH = ULSBUS_RWOI_SOT|(3<<5),
    ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL = ULSBUS_RWOI_SOT|(4<<5),
    ULSBUS_ACK_RWOI_SOT_OBJECT_CRC_MISMUCH = ULSBUS_RWOI_SOT|(5<<5),


    ULSBUS_ACK_RROI_OK = ULSBUS_RROI|(0<<5),
    ULSBUS_ACK_RROI_COMPLITE = ULSBUS_RROI|(1<<5),
    ULSBUS_ACK_RROI_OBJECT_NOTFOUND = ULSBUS_RROI|(2<<5),
    ULSBUS_ACK_RROI_OBJECT_SIZE_MISMUCH = ULSBUS_RROI|(3<<5),
    ULSBUS_ACK_RROI_OBJECT_BUFFER_FULL = ULSBUS_RROI|(4<<5),


    USBUS_ACK_AOI_SFT_OK = ULSBUS_AOI_SFT |(0<<5),
    USBUS_ACK_AOI_SFT_COMPLITE = ULSBUS_AOI_SFT |(1<<5),
    USBUS_ACK_AOI_SFT_OBJECT_NOTFOUND = ULSBUS_AOI_SFT |(2<<5),
    USBUS_ACK_AOI_SFT_OBJECT_SIZE_MISMUCH = ULSBUS_AOI_SFT |(3<<5),
    USBUS_ACK_AOI_SFT_OBJECT_BUFFER_FULL = ULSBUS_AOI_SFT |(4<<5),
    USBUS_ACK_AOI_SFT_OBJECT_CRC_MISNUCH = ULSBUS_AOI_SFT |(5<<5),

    USBUS_ACK_AOI_SOT_OK = ULSBUS_AOI_SOT |(0<<5),
    USBUS_ACK_AOI_SOT_COMPLITE = ULSBUS_AOI_SOT |(1<<5),
    USBUS_ACK_AOI_SOT_OBJECT_NOTFOUND = ULSBUS_AOI_SOT |(2<<5),
    USBUS_ACK_AOI_SOT_OBJECT_SIZE_MISMUCH = ULSBUS_AOI_SOT |(3<<5),
    USBUS_ACK_AOI_SOT_OBJECT_BUFFER_FULL = ULSBUS_AOI_SOT |(4<<5),
    USBUS_ACK_AOI_SOT_OBJECT_CRC_MISNUCH = ULSBUS_AOI_SOT |(5<<5)

}_ulsbus_ack;

typedef enum{
    ULSBUST_EMPTY,
    ULSBUST_BUSY,
    ULSBUST_BOI_TRANSMIT_START,
    ULSBUST_BOI_TRANSMIT_START_REPEAT,

    ULSBUST_BOI_TRANSMIT_F,
    ULSBUST_BOI_RECEIVE_START,
    ULSBUST_BOI_RECEIVE_F,
    ULSBUST_RECEIVE_BOI_COMPLITE,


    ULSBUST_RWOI_TRANSMIT_START,
    ULSBUST_RWOI_TRANSMIT_START_REPEAT,

    ULSBUST_RWOI_TRANSMIT_F,
    ULSBUST_RWOI_TRANSMIT_SOT_WAIT_ACK,
    ULSBUST_RWOI_TRANSMIT_COMPLITE_WAIT_ACK,

    ULSBUST_RWOI_RECEIVE_START,
    ULSBUST_RWOI_RECEIVE_F,
    ULSBUST_RWOI_RECEIVE_COMPLITE,

    ULSBUST_AOI_TRANSMIT_START,
    ULSBUST_AOI_TRANSMIT_START_REPEAT,
    ULSBUST_AOI_TRANSMIT_F,
    ULSBUST_AOI_TRANSMIT_SOT_WAIT_ACK,
    ULSBUST_AOI_TRANSMIT_COMPLITE_WAIT_ACK,

    ULSBUST_AOI_RECEIVE_START,
    ULSBUST_AOI_RECEIVE_F,
    ULSBUST_AOI_RECEIVE_COMPLITE,


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
    ULSBUS_OBJECT_FIND_OK = 0,
    ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND = 1,
    ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND = 2,
    ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH = 3,
}_ulsbus_obj_find_rezult;

typedef struct{
    uint8_t self_id;
    uint8_t remote_id;
    uint16_t dev_class;
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
        uint8_t frame_num;
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
        uint8_t frames;
        uint8_t frame_size;
        uint16_t crc;
        uint16_t size;
        uint8_t data[2];
    }__attribute__((packed))boi_sot;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t zero;
        uint8_t frameNum;
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
        uint8_t frames;
        uint8_t frame_size;
        uint16_t crc;
        uint16_t size;
    }__attribute__((packed))rwoi_sot;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t frameNum;
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
        uint8_t frames;
        uint8_t frame_size;
        uint16_t crc;
        uint16_t size;
    }__attribute__((packed))aoi_sot;
    struct{
        uint8_t cmd;
        uint8_t self_id;
        uint8_t remote_id;
        uint8_t frameNum;
        uint8_t data[8];
    }__attribute__((packed))aoi_f;

}__attribute__((packed))_ulsbus_packet;


#endif // ULSBUSTYPES_H

