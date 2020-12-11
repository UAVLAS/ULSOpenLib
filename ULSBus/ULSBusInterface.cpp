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

#include "ULSBusInterface.h"
#include "string.h"
#include "udebug.h"

#ifndef ULS_KEY_GENERATOR
#define ULS_KEY_GENERATOR() 0
#endif

ULSBusInterface::ULSBusInterface(const char* name,uint8_t id,uint8_t net):
    _name(name),
    _id(id),
    _network(net),
    _iftime(0),
    _state(IF_STATE_UNINITIALIZED),
    _key(0),
    _didx(0),
    _nm_timeout(255)
{
    rxInstance.lenght = 0;
    txInstance.lenght = 0x0;
    memset(_locals,0,IF_LOCAL_DEVICES_NUM*(sizeof(_local_device)));
}
void ULSBusInterface::task()
{
    _iftime++;
    // Check for timeout for devices
    if(_id == 255) _state = IF_STATE_UNINITIALIZED;
    switch(_state){
    case IF_STATE_UNINITIALIZED:
        if(_id == 0){
            _state = IF_STATE_OK;
        }else{
            if((_iftime%100) == 0){ // Request ID each 100ms
                sendNM_REQUESTID();
            }
        }
    case IF_STATE_OK:
        if((_iftime%10) == 0){ // each 10 ticks
            checkTimeouts();
        }
        break;
    case IF_STATE_ERROR:
        // Check error and go to IF_STATE_UNINITIALIZED
        _state = IF_STATE_UNINITIALIZED;
        break;
    }
}
void ULSBusInterface::checkTimeouts()
{
    if(_nm_timeout == 0){
        sendNM_HB();
        _nm_timeout = IF_NM_PING_HB_TIMEOUT;
    }else{
        _nm_timeout--;
    }

    for(uint32_t i=0; i<IF_LOCAL_DEVICES_NUM;i++)
    {
        if(_locals[i].timeout > 0){
            _locals[i].timeout--;
            if(_locals[i].timeout == 0){
                deviceDisconnected(i);

            }
        }
    }
}

_if_op_rezult ULSBusInterface::receivePacket()
{
    while(rxInstance.lenght == 0)
    {
        receiveBuffer();
        if(rxInstance.lenght == 0)  return IF_NO_DATA;
        uDebug("%s: !!!! Received lid: 0x%.2X cmd: 0x%.2X len: %d",_name,
               rxInstance.packet.src_lid,
               rxInstance.packet.cmd,
               rxInstance.lenght);

        if(rxInstance.packet.src_lid == _id){ // error duplicate address
            resetId();
            rxInstance.lenght = 0;
            return IF_NO_DATA;
        }
        // if it was disconnected call procedure
        if(_locals[rxInstance.packet.src_lid].timeout == 0){
            deviceConnected(rxInstance.packet.src_lid);
        }
        // Update Timeout of device
        _locals[rxInstance.packet.src_lid].timeout = IF_NM_DVICE_HB_TIMEOUT;

        if((rxInstance.packet.cmd & 0x10) == 0){ // Process NM and SYS packets
            processLocal();
            rxInstance.lenght  = 0;
            continue;
        }
        if(_state != IF_STATE_OK){ //Skip all other packets if we are not OK
            rxInstance.lenght  = 0;
        }
    }
    return IF_OK;
}
void ULSBusInterface::processLocal()
{
    switch (rxInstance.packet.cmd) {
    case IF_CMD_SYS:
        break;
    case IF_CMD_NM_GET_STATUS:
        break;
    case IF_CMD_NM_HB:
        if(rxInstance.lenght == IF_PACKET_NM_HB_SIZE + IF_PACKET_HEADER_SIZE)
            processNM_HB();
        break;
    case IF_CMD_NM_REQUEST_ID:
        if(rxInstance.lenght == IF_PACKET_NM_REQUESTID_SIZE + IF_PACKET_HEADER_SIZE)
            sendNM_SETID(rxInstance.packet.request_id.key);
        break;
    case IF_CMD_NM_SET_ID:
        if(rxInstance.lenght != IF_PACKET_NM_SETID_SIZE + IF_PACKET_HEADER_SIZE)return;
        processNM_SETID();
        break;
    case IF_CMD_NM_RESET_ID:

        break;
    }
    rxInstance.lenght  = 0;
}
_if_op_rezult ULSBusInterface::sendPacket(uint8_t cmd,uint32_t len)
{
    uDebug("%s: Send lid:0x%.2X cmd: 0x%.2X len: %d",_name,_id,cmd,len);
    if(len > IF_PAYLOAD_SIZE) return IF_ERROR;
    if((_state != IF_STATE_OK) && ((cmd&0x10) != 0))return IF_ERROR; // Only sys commands allowed

    txInstance.lenght = 0;

    txInstance.packet.cmd = cmd;
    txInstance.packet.src_lid = _id;
    txInstance.lenght = len + IF_PACKET_HEADER_SIZE;
    _if_op_rezult rez = sendBuffer();
    if(rez == IF_OK) _nm_timeout = IF_NM_PING_HB_TIMEOUT;
    return rez;
}
_if_op_rezult ULSBusInterface::sendNM_REQUESTID()
{
    _key = ULS_KEY_GENERATOR();
    txInstance.packet.request_id.key = _key;
    return sendPacket(IF_CMD_NM_REQUEST_ID,IF_PACKET_NM_REQUESTID_SIZE);
}
_if_op_rezult ULSBusInterface::sendNM_HB()
{
    txInstance.packet.hb.uid[0] = __DEVICE_UNIC_ID0;
    txInstance.packet.hb.uid[1] = __DEVICE_UNIC_ID1;
    txInstance.packet.hb.uid[2] = __DEVICE_UNIC_ID2;
    txInstance.packet.hb.uid[3] = __DEVICE_UNIC_ID3;
    return sendPacket(IF_CMD_NM_HB,IF_PACKET_NM_HB_SIZE);
}
_if_op_rezult ULSBusInterface::sendNM_SETID(uint32_t key)
{
    txInstance.packet.set_id.new_id = allocateId();
    txInstance.packet.set_id.network = _network;
    txInstance.packet.set_id.key = key;
    txInstance.lenght = IF_PACKET_NM_SETID_SIZE;
    return sendPacket(IF_CMD_NM_SET_ID,IF_PACKET_NM_SETID_SIZE);
}
uint8_t ULSBusInterface::allocateId()
{
    uint8_t free_id = 255;
    uint32_t n = IF_LOCAL_DEVICES_NUM - 1; // 0 - master device excluded
    while(n>0U)
    {
        _didx++;
        if(_didx >=255)_didx = 1; // O not allocated - it is master
        if(_locals[_didx].timeout == 0){ // slot emmpty - no device connected
            free_id = _didx;
            return free_id;
        }
        n--;
    }
    return free_id;
}
void ULSBusInterface::processNM_SETID()
{
    if(rxInstance.packet.set_id.key != _key)return;
    _id      =  rxInstance.packet.set_id.new_id;
    _network =  rxInstance.packet.set_id.network;
    _state   = IF_STATE_OK;
    sendNM_HB(); // Send PING answer;
}
void ULSBusInterface::processNM_HB()
{
    uint8_t idx = rxInstance.packet.src_lid;
    memcpy(_locals[idx].uid,rxInstance.packet.hb.uid,16);
    uDebug("%s: HB Received from 0x%.2X UID:%.8X-%.8X-%.8X-%.8X",_name,idx,
           _locals[idx].uid[0],_locals[idx].uid[1],
            _locals[idx].uid[2],_locals[idx].uid[3]);
}
void ULSBusInterface::resetId()
{
    _id = 255;
    _state = IF_STATE_UNINITIALIZED;
}

void ULSBusInterface::processSYS()
{

}



