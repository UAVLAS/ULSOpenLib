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

#define IF_PAYLOAD_SIZE (1024+5) // Need to be defined by parent protocol


#define IF_FRAME_SIZE (IF_PAYLOAD_SIZE + 2)



#define IF_NM_DVICE_HB_TIMEOUT 120
#define IF_NM_PING_HB_TIMEOUT 50

#define IF_LOCAL_DEVICES_NUM 255 // local devices 0-254 , 255 bradcast device

#define IF_PACKET_HEADER_SIZE (2)
#define IF_PACKET_SYS_SIZE (IF_PACKET_HEADER_SIZE + 2)
#define IF_PACKET_NM_HEADER_SIZE (0)
#define IF_PACKET_NM_HB_SIZE (IF_PACKET_NM_HEADER_SIZE + 16)
#define IF_PACKET_NM_REQUESTID_SIZE (IF_PACKET_NM_HEADER_SIZE + 4)
#define IF_PACKET_NM_SETID_SIZE (IF_PACKET_NM_HEADER_SIZE + 2 + 4)


typedef enum {
    IF_OK = 0,
    IF_NO_DATA ,
    IF_BUFFER_FULL,
    IF_ERROR,
    IF_ERROR_REMOTE_HOST_NOT_CONNECTED,
    IF_HOST_NOTFOUND
}_if_op_rezult;
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
                uint32_t uid[4];
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
                uint8_t  network;
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
    uint32_t  timeout;
    uint32_t uid[4];
}_local_device;

typedef struct
{
    _if_packet  packet;
    uint32_t    lenght;
}_if_instance;

class ULSBusInterface
{
public:
    ULSBusInterface(const char* name = __null,uint8_t id = 255,uint8_t net = 255);
    virtual bool open(){return false;};
    void task();
    _if_op_rezult sendPacket(uint8_t cmd,uint32_t len);
    _if_op_rezult receivePacket();
    const char* name(){return _name;};

    _if_instance rxInstance;
    _if_instance txInstance;

protected:
    virtual _if_op_rezult sendBuffer(){return IF_ERROR;};
    virtual _if_op_rezult receiveBuffer(){return IF_ERROR;};
    virtual void deviceConnected(uint8_t id){(void)id;};
    virtual void deviceDisconnected(uint8_t id){(void)id;};

private:
    // Process received packets
    void processLocal();
    void processSYS();
    void processNM_SETID();
    void processNM_HB();

    // Send packets
    _if_op_rezult sendNM_REQUESTID();
    _if_op_rezult sendNM_SETID(uint32_t key);
    _if_op_rezult sendNM_HB();

    // Utils
    void checkTimeouts();
    uint8_t allocateId();
    void resetId();
    // Gets and sets
public:
    uint8_t id(){return _id;}
    void id(uint8_t id){_id = id;}
    uint8_t network(){return _network;}
    void network(uint8_t network){_network = network;}

protected:


    const char* _name;
    uint8_t _id;
    uint8_t _network;
    uint64_t _iftime;
    _if_state _state;
    _local_device _locals[IF_LOCAL_DEVICES_NUM + 1];

private:
    uint32_t _key;
    uint32_t _didx;
    uint32_t _nm_timeout;


};

#endif // ULSBUSINTERFACE_H
