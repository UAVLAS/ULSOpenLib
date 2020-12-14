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

ULSBusInterface::ULSBusInterface(const char* name,uint8_t did):
    _name(name),
    _did(did),
    //_iftime(0),
    _state(IF_STATE_UNINITIALIZED),
    _key(0),
    _didx(0),

    _nm_timeout(0)
{

    ifRxLen = 0;
    ifTxLen = 0;
    ifRxPacket = (_if_packet*)ifRxBuf;
    ifTxPacket = (_if_packet*)ifTxBuf;

    memset(ifRxBuf,0,IF_PACKET_SIZE);
    memset(ifTxBuf,0,IF_PACKET_SIZE);
    memset(_locals,0,IF_LOCAL_DEVICES_NUM*(sizeof(_local_device)));
}
void ULSBusInterface::task(uint32_t dtms)
{
//    _iftime+=dtms;
    if(_nm_timeout >= dtms){
        _nm_timeout -=dtms;
    }else{
        _nm_timeout = 0;
    }
    // Check for timeout for devices
    if(_did == 255) _state = IF_STATE_UNINITIALIZED;
    switch(_state){
    case IF_STATE_UNINITIALIZED:
        if(_did == 0){
            _state = IF_STATE_OK;
        }else{
            if(_nm_timeout == 0){ // Request ID each 100ms
                sendNM_REQUESTID();
                _nm_timeout = IF_NM_REQUESTID_TIMEOUT;
            }
        }
        break;
    case IF_STATE_OK:
        if(_nm_timeout == 0){
            sendNM_HB();
            _nm_timeout = IF_NM_PING_HB_TIMEOUT;
        }
        for(uint32_t i=0; i<IF_LOCAL_DEVICES_NUM;i++)
        {
            if(_locals[i].timeout >= dtms){
                _locals[i].timeout -= dtms;
                if(_locals[i].timeout < dtms){
                    deviceDisconnected(i);
                }
            }else{
                _locals[i].timeout = 0;
            }
        }

        break;
    case IF_STATE_ERROR:
        // Check error and go to IF_STATE_UNINITIALIZED
        _state = IF_STATE_UNINITIALIZED;
        break;
    }
}
_io_op_rezult ULSBusInterface::ifSend()
{
    ifTxPacket->cmd |= 0x10;
    return send();
}
_io_op_rezult ULSBusInterface::ifReceive()
{
    _io_op_rezult rez = receive();
    if(rez == IO_OK)
    {
        ifRxPacket->cmd &= 0x0f;
    }
    return rez;
}
_io_op_rezult ULSBusInterface::send()
{
    if((_state != IF_STATE_OK) && ((ifTxPacket->cmd&0x10) != 0))return IO_ERROR; // Only sys commands allowed
    ifTxPacket->src_lid = _did;
    ifTxLen += IF_PACKET_HEADER_SIZE;
    uDebug("%s: Send lid:0x%.2X cmd: 0x%.2X len: %d",_name,_did,ifTxPacket->cmd,ifTxLen);
    _io_op_rezult rez = sendPacket();
    if(rez == IO_OK) _nm_timeout = IF_NM_PING_HB_TIMEOUT;
    return rez;
}

_io_op_rezult ULSBusInterface::receive()
{
    while(ifRxLen == 0)
    {
        _io_op_rezult rez = receivePacket();
        if(rez != IO_OK)  return rez;

        uDebug("%s: !!!! Received lid: 0x%.2X cmd: 0x%.2X len: %d",_name,
               ifRxPacket->src_lid,
               ifRxPacket->cmd,
               ifRxLen);
        ifRxLen -= IF_PACKET_HEADER_SIZE;
        if(ifRxPacket->src_lid == _did){ // error duplicate address
            resetId();
            ifRxLen = 0;
            return IO_NO_DATA;
        }
        // if it was disconnected call procedure
        if(_locals[ifRxPacket->src_lid].timeout == 0){
            deviceConnected(ifRxPacket->src_lid);
        }
        // Update Timeout of device
        _locals[ifRxPacket->src_lid].timeout = IF_NM_DVICE_HB_TIMEOUT;

        if((ifRxPacket->cmd & 0x10) == 0){ // Process NM and SYS packets
            processLocal();
            ifRxLen  = 0;
            continue;
        }
        if(_state != IF_STATE_OK){ //Skip all other packets if we are not OK
            ifRxLen  = 0;
        }
    }
    return IO_OK;
}
void ULSBusInterface::processLocal()
{
    switch (ifRxPacket->cmd) {
    case IF_CMD_SYS:
        break;
    case IF_CMD_NM_GET_STATUS:
        break;
    case IF_CMD_NM_HB:
        if(ifRxLen == IF_PACKET_NM_HB_SIZE)
            processNM_HB();
        break;
    case IF_CMD_NM_REQUEST_ID:
        if(ifRxLen == IF_PACKET_NM_REQUESTID_SIZE)
            sendNM_SETID(ifRxPacket->request_id.key);
        break;
    case IF_CMD_NM_SET_ID:
        if(ifRxLen != IF_PACKET_NM_SETID_SIZE )return;
        processNM_SETID();
        break;
    case IF_CMD_NM_RESET_ID:

        break;
    }
    ifRxLen  = 0;
}

_io_op_rezult ULSBusInterface::sendNM_REQUESTID()
{
    ifTxPacket->cmd = IF_CMD_NM_REQUEST_ID;
    ifTxLen = IF_PACKET_NM_REQUESTID_SIZE;

    _key = ULS_KEY_GENERATOR();
    ifTxPacket->request_id.key = _key;
    return send();
}
_io_op_rezult ULSBusInterface::sendNM_HB()
{
    ifTxPacket->cmd = IF_CMD_NM_HB;
    ifTxLen = IF_PACKET_NM_HB_SIZE;

    ifTxPacket->hb.uid0 = __DEVICE_UNIC_ID0;
    return send();
}
_io_op_rezult ULSBusInterface::sendNM_SETID(uint32_t key)
{
    ifTxPacket->cmd = IF_CMD_NM_SET_ID;
    ifTxLen = IF_PACKET_NM_SETID_SIZE;

    ifTxPacket->set_id.new_id = allocateId();
    ifTxPacket->set_id.key = key;
    return send();
}
uint8_t ULSBusInterface::allocateId()
{
    uint8_t free_did = 255;
    uint32_t n = IF_LOCAL_DEVICES_NUM - 1; // 0 - master device excluded
    while(n>0U)
    {
        _didx++;
        if(_didx >=255)_didx = 1; // O not allocated - it is master
        if(_locals[_didx].timeout == 0){ // slot emmpty - no device connected
            free_did = _didx;
            return free_did;
        }
        n--;
    }
    return free_did;
}
void ULSBusInterface::processNM_SETID()
{
    if(ifRxPacket->set_id.key != _key)return;
    _did      =  ifRxPacket->set_id.new_id;
    _state   = IF_STATE_OK;
    sendNM_HB(); // Send PING answer;
    ifOk();
}
void ULSBusInterface::processNM_HB()
{
    uint8_t idx = ifRxPacket->src_lid;
    _locals[idx].uid0 = ifRxPacket->hb.uid0;
    uDebug("%s: HB Received from 0x%.2X UID0:%.8X",_name,idx,
           _locals[idx].uid0);
}
void ULSBusInterface::resetId()
{
    _did = 255;
    _state = IF_STATE_UNINITIALIZED;
}

void ULSBusInterface::processSYS()
{

}



