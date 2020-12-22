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

#ifndef ULSBUSINTERFACE_H
#define ULSBUSINTERFACE_H

#include "ULSBusTypes.h"

#define IF_PACKET_SIZE 1324

#define IF_PACKET_HEADER_SIZE (2)
#define IF_PAYLOAD_SIZE (IF_PACKET_SIZE - IF_PACKET_HEADER_SIZE)

#define IF_NM_DVICE_HB_TIMEOUT 1000
#define IF_NM_PING_HB_TIMEOUT 400
#define IF_NM_REQUESTID_TIMEOUT 100

#define IF_LOCAL_DEVICES_NUM 63 // local devices 0-62 , 63 bradcast device

#define IF_PACKET_SYS_SIZE (2)
#define IF_PACKET_NM_HEADER_SIZE (0)
#define IF_PACKET_NM_HB_SIZE (IF_PACKET_NM_HEADER_SIZE + 4)
#define IF_PACKET_NM_REQUESTID_SIZE (IF_PACKET_NM_HEADER_SIZE + 4)
#define IF_PACKET_NM_SETID_SIZE (IF_PACKET_NM_HEADER_SIZE + 1 + 4)


typedef enum {
    IO_OK = 0,
    IO_NO_DATA ,
    IO_BUFFER_FULL,
    IO_ERROR,
    IO_ERROR_REMOTE_HOST_NOT_CONNECTED,
    IO_HOST_NOTFOUND
}_io_op_rezult;
typedef enum {
    IF_CMD_SYS = 0,
    IF_CMD_NM_HB = 1,
    IF_CMD_NM_GET_STATUS = 2,
    IF_CMD_NM_REQUEST_ID = 3,
    IF_CMD_NM_SET_ID = 4,
    IF_CMD_NM_RESET_ID = 5
}_if_cmd;


typedef enum {
    IF_STATE_UNINITIALIZED = 0,
    IF_STATE_OK = 1,
    IF_STATE_ERROR = 3,
}_if_state;


// Packet structure
typedef struct{
    uint8_t cmd;
    uint8_t src_lid;
    union{
        //Sysmem messges
        struct{
            uint8_t  dsn_lid;
            uint8_t  param;
        }__attribute__((packed))sys;
        //Network messges
        union{
            // HB packets
            struct{
                uint32_t uid0;
            }__attribute__((packed))hb;
            // status
            struct{
                uint8_t dsn_lid;
            }__attribute__((packed))get_status;
            // Request ID
            struct{
                uint32_t key;
            }__attribute__((packed))request_id;
            // Set Id
            struct{
                uint8_t  new_id;
                uint32_t key;
            }__attribute__((packed))set_id;
        }__attribute__((packed));
        //Buffer direct access
        uint8_t  pld[IF_PAYLOAD_SIZE]; // Payload of if_packet
    }__attribute__((packed));

}__attribute__((packed))_if_packet;


typedef struct
{
    uint8_t  id;
    uint32_t timeout;
    uint32_t uid0;
}_local_device;

class ULSBusInterface
{
public:
    ULSBusInterface(const char* name = __null,uint8_t did = IF_LOCAL_DEVICES_NUM);
    virtual bool open(){return false;};
    void task(uint32_t dtms);

    _io_op_rezult ifSend();
    _io_op_rezult ifReceive();
    uint8_t ifid(){return (_did & 0x3f);}
    void ifid(uint8_t did){_did = (did & 0x3f);}

    const char* name(){return _name;};

    _if_packet *ifRxPacket;
    _if_packet *ifTxPacket;

    uint32_t ifRxLen;
    uint32_t ifTxLen;
    uint8_t  ifRxBuf[IF_PACKET_SIZE];
    uint8_t  ifTxBuf[IF_PACKET_SIZE];

protected:
    virtual _io_op_rezult sendPacket(){return IO_ERROR;};
    virtual _io_op_rezult receivePacket(){return IO_ERROR;};
    virtual void deviceConnected(uint8_t id){(void)id;};
    virtual void deviceDisconnected(uint8_t id){(void)id;};
    virtual void ifOk(){};


private:
    _io_op_rezult send();
    _io_op_rezult receive();

    // Process received packets
    void processLocal();
    void processSYS();
    void processNM_SETID();
    void processNM_HB();

    // Send packets
    _io_op_rezult sendNM_REQUESTID();
    _io_op_rezult sendNM_SETID(uint32_t key);
    _io_op_rezult sendNM_HB();

    // Utils
    uint8_t allocateId();
    void resetId();

protected:
    const char* _name;
    uint8_t _did; // Local device Id
    // uint64_t _iftime;
    _if_state _state;
    _local_device _locals[IF_LOCAL_DEVICES_NUM + 1];

private:
    uint32_t _key;
    uint32_t _didx;
    uint32_t _nm_timeout;

};

#endif // ULSBUSINTERFACE_H
