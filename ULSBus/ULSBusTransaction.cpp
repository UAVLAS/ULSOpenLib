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

#include "ULSBusTransaction.h"

ULSBusTransaction::ULSBusTransaction():
    ULSListItem(),
    _library(__null),
    _state(ULSBUST_EMPTY),
    _timeout(0),
    _cmd(0),
    _self_id(0),
    _remote_id(0),
    _frames(0),
    _frame_idx(0),
    _frame_size(0),
    _frame_size_last(0)
{
    close();
}
void ULSBusTransaction::library(ULSDevicesLibrary    *library)
{
    _library = library;
}
bool ULSBusTransaction::open(ULSBusConnection* connection,uint8_t selfId,uint8_t remoteId)
{
    if(_state != ULSBUST_EMPTY) return false;
    _state = ULSBUST_BUSY;
    _connection = connection;
    _self_id = selfId;
    _remote_id = remoteId;
    _buf = __null;
    _timeout = ULSBUS_TIMEOUT;
    return true;
}

void ULSBusTransaction::close()
{
    _state = ULSBUST_EMPTY;
    _cmd = 0;
    _frames = 0;
    _frame_size = 0;
    _frame_size_last = 0;
    _frame_idx = 0;
    disconnectBuffer();
    _connection = __null;
}
void ULSBusTransaction::close(uint8_t cmd,uint8_t remoteID)
{
    if((cmd == _cmd)&&(remoteID == _remote_id)) close();
}

void ULSBusTransaction::connectBuffer(ULSBusObjectBuffer* buf)
{
    _buf = buf;
    buf->connect();
}
void ULSBusTransaction::disconnectBuffer()
{
    if(!_buf)return;
    _buf->disconnect();
    _buf = __null;
}
void ULSBusTransaction::initFrames()
{
    _frame_size = _connection->maxFrameSize();
    _frames = _buf->size()/_frame_size;
    _frame_idx = 0;
    _frame_size_last = _buf->size()%_frame_size;
    if(_frame_size_last == 0){
        _frame_size_last = _frame_size;
    }else{
        _frames++; // Add incomplete frame
    }
}
bool ULSBusTransaction::check(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id,_ulsbus_transaction_state state)
{
    if((connection == _connection)&&
            (self_id == _self_id)&&
            (remote_id == _remote_id)&&
            (state == _state)) return true;
    return false;
}
//===================================================
bool ULSBusTransaction::boiTransmitStart(){
    _ulsbus_packet  *pxTxPack = (_ulsbus_packet *)(_connection->interface()->txBufInstance.buf);
    if(!_buf){
        close();
        return false;
    }
    if(_buf->size() == 0){
        close();
        return false;
    }

    if(_buf->size() <= _connection->maxFrameSize()){
        // Transmit data SFT using interface buffer
        _cmd = ULSBUS_BOI_SFT;
        pxTxPack->boi_sft.cmd = ULSBUS_BOI_SFT;
        pxTxPack->boi_sft.self_id = _self_id;
        pxTxPack->boi_sft.obj_id = _buf->id();
        if(_buf->getData(_connection->maxFrameSize(),0,pxTxPack->boi_sft.data,_buf->size())){ // If buffer ready
            _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_BOI_SFT + _buf->size();
            if(_connection->interface()->send()){
                // Close transaction no need to wait answer
                close();
            }else{
                // Error interface buffer full
                ULSBUS_ERROR("Sending ULSBUS_BOI_SFT FAIL : self_id: 0x%X remote_id: 0x%X ",_self_id,_remote_id);
                _state = ULSBUST_BOI_TRANSMIT_START_REPEAT;
            }
        }
    }else{
        // Prepare for transmittion frames.
        initFrames();
        _cmd = ULSBUS_BOI_SOT;
        // Transmit SOT.
        pxTxPack->boi_sot.cmd = ULSBUS_BOI_SOT;
        pxTxPack->boi_sot.self_id = _self_id;
        pxTxPack->boi_sot.obj_id = _buf->id();
        pxTxPack->boi_sot.frame_last_idx = _frames - 1;
        pxTxPack->boi_sot.byte_last_idx = _frame_size - 1;
        pxTxPack->boi_sot.obj_size = _buf->size();
        pxTxPack->boi_sot.crc = 0;//TODO: _buf->(crc);
        _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_BOI_SOT;

        if(_connection->interface()->send()){
            _state = ULSBUST_BOI_TRANSMIT_F;
        }else{
            // Error interface buffer full try to resend data later
            ULSBUS_ERROR("Sending ULSBUS_BOI_SOT FAIL : self_id: 0x%X remote_id: 0x%X ",_self_id,_remote_id);
            _state = ULSBUST_BOI_TRANSMIT_START_REPEAT;
        }
    }
    return false;
}
//===================================================
bool ULSBusTransaction::rwoiTransmitStart(){
    _ulsbus_packet  *pxTxPack = (_ulsbus_packet *)(_connection->interface()->txBufInstance.buf);
    if(_buf == __null){
        close();
        return false;
    }
    if(_buf->size() == 0){
        close();
        return false;
    }
    if((_buf->size() + 2) <= _connection->maxFrameSize()){
        // Transmit data SFT
        _cmd = ULSBUS_RWOI_SFT;
        pxTxPack->rwoi_sft.cmd = ULSBUS_RWOI_SFT;
        pxTxPack->rwoi_sft.self_id = _self_id;
        pxTxPack->rwoi_sft.remote_id = _remote_id;
        pxTxPack->rwoi_sft.obj_id = _buf->id();
        if(_buf->getData(_connection->maxFrameSize(),0,pxTxPack->rwoi_sft.data,_buf->size())){ // If buffer ready
            _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_SFT + _buf->size();
            if(_connection->interface()->send())
            {
                _state = ULSBUST_TRANSMIT_COMPLITE_WAIT_ACK;
                _timeout = ULSBUS_TIMEOUT; // keep active
            }else{
                // Error interface buffer full repeat later
                ULSBUS_ERROR("Sending ULSBUS_RWOI_SFT FAIL : self_id: 0x%X remote_id: 0x%X ",_self_id,_remote_id);
                _state = ULSBUST_RWOI_TRANSMIT_START_REPEAT;
            }
        }
    }else{
        // Prepare for transmittion frames.
        initFrames();
        _cmd = ULSBUS_RWOI_SOT;
        // Transmit data SOT
        pxTxPack->rwoi_sot.cmd = ULSBUS_RWOI_SOT;
        pxTxPack->rwoi_sot.self_id = _self_id;
        pxTxPack->rwoi_sot.remote_id = _remote_id;
        pxTxPack->rwoi_sot.obj_id = _buf->id();
        pxTxPack->rwoi_sot.frame_last_idx = _frames - 1;
        pxTxPack->rwoi_sot.byte_last_idx  = _frame_size - 1;
        pxTxPack->rwoi_sot.obj_size = _buf->size();
        pxTxPack->rwoi_sot.crc = 0;//_buf->(crc);
        pxTxPack->rwoi_sot.zero = 0;
        _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_SOT;

        if(_connection->interface()->send()){
            _state = ULSBUST_RWOI_TRANSMIT_F;
        }else{
            // Error interface buffer full repeat later
             ULSBUS_ERROR("Sending ULSBUS_RWOI_SOT FAIL : self_id: 0x%X remote_id: 0x%X ",_self_id,_remote_id);
            _state = ULSBUST_RWOI_TRANSMIT_START_REPEAT;
        }
    }
    return false;
}
//===================================================
bool ULSBusTransaction::aoiTransmitStart(){
    if(!_connection->interface()){
     ULSBUS_ERROR("Interface id __null",0);
    }
    _ulsbus_packet  *pxTxPack = (_ulsbus_packet *)(_connection->interface()->txBufInstance.buf);

    if(!_buf){
        close();
        return false;
    }
    if(_buf->size() == 0){
        close();
        return false;
    }
    if((_buf->size() + 2) <= _connection->maxFrameSize()){
        // Transmit data SFT
        _cmd = ULSBUS_AOI_SFT;
        pxTxPack->aoi_sft.cmd = ULSBUS_AOI_SFT;
        pxTxPack->aoi_sft.self_id = _self_id;
        pxTxPack->aoi_sft.remote_id = _remote_id;
        pxTxPack->aoi_sft.obj_id = _buf->id();

        if(_buf->getData(_connection->maxFrameSize(),0,pxTxPack->aoi_sft.data,_buf->size())){ // If buffer ready
            _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_AOI_SFT + _buf->size();
            if(_connection->send())
            {
                _state = ULSBUST_TRANSMIT_COMPLITE_WAIT_ACK;
                _timeout = ULSBUS_TIMEOUT; // keep active
            }else{
                // Error interface buffer full repeat later
                _state = ULSBUST_AOI_TRANSMIT_START_REPEAT;
                ULSBUS_ERROR("Sending ULSBUS_AOI_SFT Connection failure : self_id: 0x%X remote_id: 0x%X ",_self_id,_remote_id);
            }
        }else{
            //int i =10;
            _state = ULSBUST_AOI_TRANSMIT_START_REPEAT;
            ULSBUS_ERROR("Sending ULSBUS_AOI_SFT Buffer Not ready : self_id: 0x%X remote_id: 0x%X ",_self_id,_remote_id);
            return false; // Buffer Not ready
        }
    }else{
        // Prepare for transmittion frames.
        initFrames();
        _cmd = ULSBUS_AOI_SOT;
        // Transmit data SOT
        pxTxPack->aoi_sot.cmd = ULSBUS_AOI_SOT;
        pxTxPack->aoi_sot.self_id = _self_id;
        pxTxPack->aoi_sot.remote_id = _remote_id;
        pxTxPack->aoi_sot.obj_id = _buf->id();
        pxTxPack->aoi_sot.frame_last_idx = _frames - 1;
        pxTxPack->aoi_sot.byte_last_idx = _frame_size - 1;
        pxTxPack->aoi_sot.size = _buf->size();
        pxTxPack->aoi_sot.crc = 0;//_buf->(crc);
        _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_AOI_SOT;

        if(_connection->send()){
            _state = ULSBUST_AOI_TRANSMIT_F;
            _timeout = ULSBUS_TIMEOUT; // keep active
        }else{
            // Error interface buffer full repeat later
            ULSBUS_ERROR("Sending ULSBUS_AOI_SOT FAIL : self_id: 0x%X remote_id: 0x%X ",_self_id,_remote_id);
            _state = ULSBUST_AOI_TRANSMIT_START_REPEAT;
        }
    }
    return false;
}
void ULSBusTransaction::frameReceived(uint32_t frame_idx,uint8_t *buf)
{
    _frame_idx = frame_idx;
    if((_frame_idx + 1) == _frames)
    {
        _buf->setData(_frame_size,_frame_idx,buf,_frame_size_last);
    }else{
        _buf->setData(_frame_size,_frame_idx,buf,_frame_size);
    }
}
bool ULSBusTransaction::frameTransmit(uint8_t *buf)
{
    uint32_t size = ((_frame_idx + 1)  ==  _frames)?_frame_size_last:_frame_size; // set size if it is  last frame

    if(!_buf->getData(_frame_size,_frame_idx,buf,size))return true; // Exit if no data aviable;
    switch(_cmd)
    {
    case ULSBUS_BOI_F: _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_BOI_F +  size;
        break;
    case ULSBUS_RWOI_F: _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_F +  size;
        break;
    case ULSBUS_AOI_F: _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_AOI_F +  size;
        break;
    }

    if(_connection->interface()->send())
    {
        _timeout = ULSBUS_TIMEOUT; // keep active
        if((_frame_idx + 1) == _frames){// Last frame was sended
            if(_cmd != ULSBUS_BOI_F){
                _state = ULSBUST_TRANSMIT_COMPLITE_WAIT_ACK;
            }else {
                close();
            }
        }else{
            _frame_idx++;
        }
        return true;
    }else{
        // IDLE
        return false; // It will try to resend frame on next Task
    }
}
bool ULSBusTransaction::frameBufferCheck(bool broadcast)
{
    uint8_t self   = (broadcast)?_self_id:0;
    uint8_t remote = (!broadcast)?_remote_id:0;

    if(_buf->isBufferComlite())
    {
        _ulsbus_obj_find_rezult rez = _library->find(remote,self,_buf->id(),_buf->size()); //m self_id = 0 -> from any devices
        if(rez == ULSBUS_OBJECT_FIND_OK){
            ULSBusObjectBase *obj =  _library->getObject(remote,self,_buf->id()); // Looking for object in library
            if(!obj)return false; // WTF ?
            obj->setData(_buf->pxBuf());
            if(!broadcast)_connection->sendAck(ULSBUS_ACK_COMPLITE,_cmd,_self_id,_remote_id);
        }
        if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){
            // Do tothing not pur device - close transaction an leave buffer for others transactions.
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND){
            if(!broadcast)_connection->sendAck(ULSBUS_ACK_OBJECT_NOTFOUND,_cmd,_self_id,_remote_id);
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH){
            if(!broadcast)_connection->sendAck(ULSBUS_ACK_OBJECT_SIZE_MISMUTCH,_cmd,_self_id,_remote_id);
        }

        close(); // Buffer ok close connection
    }else{
        // Check buffer if frame missed send request for lost frame
        if(_timeout == ULSBUS_TIMEOUT/2){
            // request lost frames
        }
    }
    return true;
}
//================================================================
//================================================================
bool ULSBusTransaction::processPacket()
{
    if(_state == ULSBUST_EMPTY)return false;
    if(!_connection)return false;
    _ulsbus_packet  *pxRxPack = (_ulsbus_packet *)(_connection->interface()->rxBufInstance.buf);
    // some package received in this transaction
    _timeout = ULSBUS_TIMEOUT; // keep active

    switch(_state){
    //================================================================
    // Start Transmitting the package
    case ULSBUST_BOI_TRANSMIT_START:
        return boiTransmitStart();
        break;
    case ULSBUST_RWOI_TRANSMIT_START:
        return rwoiTransmitStart();
        break;
    case ULSBUST_AOI_TRANSMIT_START:
        return aoiTransmitStart();
        break;
        //================================================================
        // Start Receiving the package
    case ULSBUST_BOI_RECEIVE_START:{
        _cmd = ULSBUS_BOI_SOT;
        _frames = (uint32_t)pxRxPack->boi_sot.frame_last_idx + 1;
        _frame_size = (uint32_t)pxRxPack->boi_sot.byte_last_idx + 1; // 0 - 1frame, 1 - 2 frames ... 255 = 256 frames.
        _self_id = pxRxPack->boi_sot.self_id;
        _remote_id = 0;
        _frame_size_last = pxRxPack->boi_sot.obj_size % _frame_size;
        if(_frame_size_last == 0){
            _frame_size_last = _frame_size;
        }
        _state = ULSBUST_BOI_RECEIVE_F;
    }
        break;
    case ULSBUST_RWOI_RECEIVE_START:{
        _cmd = ULSBUS_RWOI_SOT;
        _frames = (uint32_t)pxRxPack->rwoi_sot.frame_last_idx + 1;
        _frame_size = (uint32_t)pxRxPack->rwoi_sot.byte_last_idx + 1; // 0 - 1frame, 1 - 2 frames ... 255 = 256 frames.
        _self_id = pxRxPack->rwoi_sot.self_id;
        _remote_id = pxRxPack->rwoi_sot.remote_id;
        _frame_size_last = pxRxPack->rwoi_sot.obj_size % _frame_size;
        if(_frame_size_last == 0){
            _frame_size_last = _frame_size;
        }
        _state = ULSBUST_RWOI_RECEIVE_F;
    }
        break;
    case ULSBUST_AOI_RECEIVE_START:{
        _cmd = ULSBUS_AOI_SOT;
        _frames = (uint32_t)pxRxPack->aoi_sot.frame_last_idx + 1;
        _frame_size = (uint32_t)pxRxPack->aoi_sot.byte_last_idx + 1;
        _self_id = pxRxPack->aoi_sot.self_id;
        _remote_id = pxRxPack->aoi_sot.remote_id;
        _frame_size_last = pxRxPack->aoi_sot.size % _frame_size;
        if(_frame_size_last == 0){
            _frame_size_last = _frame_size;
        }
        _state = ULSBUST_AOI_RECEIVE_F;
    }
        break;
        //================================================================
        // Receiving the frame
    case ULSBUST_BOI_RECEIVE_F:
        frameReceived(pxRxPack->boi_f.frame_idx,pxRxPack->boi_f.data);
        break;
    case ULSBUST_RWOI_RECEIVE_F:
        frameReceived(pxRxPack->rwoi_f.frame_idx,pxRxPack->rwoi_f.data);
        break;
    case ULSBUST_AOI_RECEIVE_F:
        frameReceived(pxRxPack->aoi_f.frame_idx,pxRxPack->aoi_f.data);
        break;
        //================================================================
        // Transmit complite Packet processing

    case ULSBUST_TRANSMIT_COMPLITE_WAIT_ACK:
        close(); // Anyway close connection.
        if((pxRxPack->ack.ackcmd >> 5) == ULSBUS_ACK_COMPLITE){
            return true;
        };
        return false; // ACK with error received.
        break;

    default:
        break;

    }
    return true;
}
//================================================================
//================================================================
bool ULSBusTransaction::task() // calls every ms
{

    if(_timeout>0){
        _timeout--;
        if(_timeout == 0){
            close(); // Close transaction by timeout
            return false;
        }
    }

    if(_state == ULSBUST_EMPTY)return false;
    if(_connection == __null)return false;
    _ulsbus_packet  *pxTxPack = (_ulsbus_packet *)(_connection->interface()->txBufInstance.buf);

    switch(_state){
    //================================================================
    // Transmit start repeat

    case ULSBUST_BOI_TRANSMIT_START_REPEAT:
        boiTransmitStart(); // Try to start transmittion until timeout
        break;
    case ULSBUST_RWOI_TRANSMIT_START_REPEAT:
        rwoiTransmitStart(); // Try to start transmittion until timeout
        break;
    case ULSBUST_AOI_TRANSMIT_START_REPEAT:
        aoiTransmitStart(); // Try to start transmittion until timeout
        break;
        //================================================================
        // Transmit frames
    case ULSBUST_BOI_TRANSMIT_F:
        _cmd = ULSBUS_BOI_F;
        pxTxPack->boi_f.cmd = _cmd;
        pxTxPack->boi_f.frame_idx = _frame_idx;
        pxTxPack->boi_f.self_id = _self_id;
        pxTxPack->boi_f.zero = 0;
        frameTransmit(pxTxPack->boi_f.data);
        break;
    case ULSBUST_RWOI_TRANSMIT_F:
        _cmd = ULSBUS_RWOI_F;
        pxTxPack->rwoi_f.cmd = _cmd;
        pxTxPack->rwoi_f.frame_idx = _frame_idx;
        pxTxPack->rwoi_f.self_id = _self_id;
        pxTxPack->rwoi_f.remote_id = _remote_id;
        frameTransmit(pxTxPack->rwoi_f.data);
        break;
    case ULSBUST_AOI_TRANSMIT_F:
        _cmd = ULSBUS_AOI_F;
        pxTxPack->aoi_f.cmd = _cmd;
        pxTxPack->aoi_f.frame_idx = _frame_idx;
        pxTxPack->aoi_f.self_id = _self_id;
        pxTxPack->aoi_f.remote_id = _remote_id;
        frameTransmit(pxTxPack->aoi_f.data);
        break;

        //================================================================
        // Receive frames
    case ULSBUST_BOI_RECEIVE_F:
        frameBufferCheck(true);
        break;
    case ULSBUST_RWOI_RECEIVE_F:
        frameBufferCheck(false);
        break;
    case ULSBUST_AOI_RECEIVE_F:
        frameBufferCheck(false);
        break;
    default:
        break;
    }
    return true;
}


void ULSBusTransaction::state(_ulsbus_transaction_state state)
{
    _state = state;
}
_ulsbus_transaction_state ULSBusTransaction::state()
{
    return _state;
}

ULSBusObjectBuffer* ULSBusTransaction::buffer()
{
    return _buf;
};

ULSBusTransactionsList::ULSBusTransactionsList():ULSList()
{
    ULSBusTransaction *px = head();
    while(px){
        px->close();
        px = forward(px);
    };
};

void ULSBusTransactionsList::library(ULSDevicesLibrary    *library){
    ULSBusTransaction *px = head();
    while(px){
        px->library(library);
        px = forward(px);
    };
}
void ULSBusTransactionsList::task(){
    ULSBusTransaction *px = head();
    while(px){
        px->task();
        px = forward(px);
    };
}
ULSBusTransaction* ULSBusTransactionsList::open(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id)
{
    ULSBusTransaction *px = head();
    while(px){
        if(px->open(connection,self_id,remote_id) )return px;
        px = forward(px);
    };
    return __null;
}

ULSBusTransaction* ULSBusTransactionsList::open(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id,ULSBusObjectBuffer* buf,_ulsbus_transaction_state state)
{

    ULSBusTransaction *px = head();
    while(px){
        if(px->open(connection,self_id,remote_id) ){
            if(buf){
                px->connectBuffer(buf);
            }
            px->state(state);
            px->processPacket();
            return px;
        }
        px = forward(px);
    };
    ULSBUS_ERROR("Transaction Open FAIL : self_id: 0x%X remote_id: 0x%X ",self_id,remote_id);
    return __null;
}

ULSBusTransaction* ULSBusTransactionsList::find(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id,_ulsbus_transaction_state state)
{
    ULSBusTransaction *px = head();
    while(px){
        if(px->check(connection,self_id,remote_id,state))return px;
        px = forward(px);
    };
    return __null;
}
uint32_t ULSBusTransactionsList::openedTransactions()
{
    uint32_t n = 0;
    ULSBusTransaction *px = head();
    while(px){
        if(px->state()!=ULSBUST_EMPTY)n++;
        px = forward(px);
    };
    return n;
}
