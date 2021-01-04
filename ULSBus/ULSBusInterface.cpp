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

#ifndef ULS_KEY_GENERATOR
#define ULS_KEY_GENERATOR (__DEVICE_UNIC_ID3^__DEVICE_UNIC_ID2^__DEVICE_UNIC_ID1^__DEVICE_UNIC_ID0)
#endif

ULSBusInterface::ULSBusInterface(const char* name,uint8_t did):
    ifclbkBlitzReceived(nullptr),
    _name(name),
    _did(did),
    _state(IF_STATE_UNINITIALIZED),
    _key(0),
    _key_cntr(0),
    _didx(0),
    _nm_timeout(0)
{
    ifRxLen = 0;
    ifTxLen = 0;
    ifRxPacket = (_if_packet*)ifRxBuf;
    ifTxPacket = (_if_packet*)ifTxBuf;

    if(_did>0x3f)_did=0x3f;

    memset(ifRxBuf,0,IF_PACKET_SIZE);
    memset(ifTxBuf,0,IF_PACKET_SIZE);
    memset(_locals,0,IF_LOCAL_DEVICES_NUM*(sizeof(_local_device)));
}
void ULSBusInterface::task(uint32_t dtms)
{
    _key_cntr++;
    if(_nm_timeout >= dtms){
        _nm_timeout -=dtms;
    }else{
        _nm_timeout = 0;
    }
    // Check for timeout for devices
    if(_did >= 0x3f) _state = IF_STATE_UNINITIALIZED;
    switch(_state){
    case IF_STATE_UNINITIALIZED:
        if(_did == 0){
            _state = IF_STATE_OK;
            ifOk();
        }else{
            if(_nm_timeout == 0){ // Request ID each IF_NM_REQUESTID_TIMEOUT
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
    if(rez == IO_OK){
        ifRxPacket->cmd &= 0x0f;
    }
    return rez;
}
_io_op_rezult ULSBusInterface::send()
{
    if((_state != IF_STATE_OK) && ((ifTxPacket->cmd&0x10) != 0))return IO_ERROR; // Only sys commands allowed
    ifTxPacket->src_lid = _did;
    ifTxLen += IF_PACKET_HEADER_SIZE;
    // DEBUG_MSG("%s: Send lid:0x%.2X cmd: 0x%.2X len: %d",_name,_did,ifTxPacket->cmd,ifTxLen);
    // if(ifTxPacket->cmd!= IF_CMD_NM_HB)DEBUG_PACKET(_name,"Tx",ifTxBuf,ifTxLen);
    _io_op_rezult rez = sendPacket();
    if(rez == IO_OK) _nm_timeout = IF_NM_PING_HB_TIMEOUT;
    return rez;
}
_io_op_rezult ULSBusInterface::receive()
{
    while(ifRxLen == 0){
        _io_op_rezult rez = receivePacket();
        if(rez != IO_OK)  return rez;
        //        DEBUG_MSG("%s: Received lid: 0x%.2X cmd: 0x%.2X len: %d",_name,
        //               ifRxPacket->src_lid,
        //               ifRxPacket->cmd,
        //               ifRxLen);
        //if(ifRxPacket->cmd!= IF_CMD_NM_HB) DEBUG_PACKET(_name,"Rx",ifRxBuf,ifRxLen);

        ifRxLen -= IF_PACKET_HEADER_SIZE;
        if(ifRxPacket->src_lid == _did){ // error duplicate address
            resetId();
            ifRxLen = 0;
            return IO_NO_DATA;
        }
        // if it was disconnected call procedure
        if(_locals[ifRxPacket->src_lid].timeout == 0){
            if(ifRxPacket->src_lid < 0x3f)
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
        if(ifRxLen == IF_PACKET_NM_HB_SIZE)processNM_HB();
        break;
    case IF_CMD_NM_REQUEST_ID:
        if((ifRxLen == IF_PACKET_NM_REQUESTID_SIZE) && (_did == 0x0))sendNM_SETID(ifRxPacket->request_id.key);
        break;
    case IF_CMD_NM_SET_ID:
        if(ifRxLen == IF_PACKET_NM_SETID_SIZE )processNM_SETID();
        break;
    case IF_CMD_NM_RESET_ID:

        break;
    case IF_CMD_BLITZ:
        IF_CALL(ifclbkBlitzReceived);
        break;
    }
    ifRxLen  = 0;
}

_io_op_rezult ULSBusInterface::sendNM_REQUESTID()
{
    ifTxPacket->cmd = IF_CMD_NM_REQUEST_ID;
    ifTxLen = IF_PACKET_NM_REQUESTID_SIZE;

    _key = ULS_KEY_GENERATOR ^ _key_cntr;
    // DEBUG_MSG("%s: send REQUEST ID lid:0x%.2X KEY: 0x%.4X ",_name,_did, _key);
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
    // DEBUG_MSG("%s: send SET ID lid:0x%.2X NEWID: 0x%.2X KEY: 0x%.4X",_name,_did, ifTxPacket->set_id.new_id,key);
    ifTxPacket->set_id.key = key;
    return send();
}
_io_op_rezult ULSBusInterface::ifSendBLITZ(uint16_t blitz_id,uint8_t *buf,uint32_t size)
{
    if(size>8)return IO_ERROR;
    ifTxPacket->cmd = IF_CMD_BLITZ;
    ifTxLen = IF_PACKET_BLITZ_SIZE + size;
    ifTxPacket->blitz.id = blitz_id;
    memcpy(ifTxPacket->blitz.data,buf,size);
    return send();
}

uint8_t ULSBusInterface::allocateId()
{
    uint32_t n = IF_LOCAL_DEVICES_NUM - 1; // 0 - master device excluded
    while(n>0U){
        _didx++;
        if(_didx >=IF_LOCAL_DEVICES_NUM)_didx = 1; // O not allocated - it is master
        if(_locals[_didx].timeout == 0){ // slot emmpty - no device connected
            return _didx;
        }
        n--;
    }
    return IF_LOCAL_DEVICES_NUM;
}
void ULSBusInterface::processNM_SETID()
{
    // DEBUG_MSG("%s: Received SET ID new ID:0x%.2X REMKEY:0x%.4X  OUR KEY: 0x%.4X",_name,ifTxPacket->set_id.new_id,ifRxPacket->set_id.key,_key);
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
}
void ULSBusInterface::resetId()
{
    _did = IF_LOCAL_DEVICES_NUM;
    _state = IF_STATE_UNINITIALIZED;
}

void ULSBusInterface::processSYS()
{

}



