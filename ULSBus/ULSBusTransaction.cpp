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
    _frameNum(0),
    _frameSize(0),
    _frameLastSize(0)
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
    _frameSize = 0;
    _frameLastSize = 0;
    _frameNum = 0;
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
    _frameSize = _connection->maxFrameSize();
    _frames = _buf->size()/_frameSize;
    _frameNum = 0;
    _frameLastSize = _buf->size()%_frameSize;
    if(_frameLastSize){
        _frames++;
    }else{
        _frameLastSize = _frameSize;
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
    _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_connection->interface()->txBufInstance.buf);
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
        pxPack->boi_sft.cmd = ULSBUS_BOI_SFT;
        pxPack->boi_sft.self_id = _self_id;
        pxPack->boi_sft.obj_id = _buf->id();
        if(_buf->getData(_connection->maxFrameSize(),0,pxPack->boi_sft.data,_buf->size())){ // If buffer ready
            _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_BOI_SFT + _buf->size();
            if(_connection->interface()->send()){
                // Close transaction no need to wait answer
                close();
            }else{
                // Error interface buffer full
                _state = ULSBUST_BOI_TRANSMIT_START_REPEAT;
            }
        }
    }else{
        // Prepare for transmittion frames.
        initFrames();
        _cmd = ULSBUS_BOI_SOT;
        // Transmit SOT.
        pxPack->boi_sot.cmd = ULSBUS_BOI_SOT;
        pxPack->boi_sot.self_id = _self_id;
        pxPack->boi_sot.obj_id = _buf->id();
        pxPack->boi_sot.frames = _frames;
        pxPack->boi_sot.frame_size = _frameSize;
        pxPack->boi_sot.size = _buf->size();
        pxPack->boi_sot.crc = 0;//TODO: _buf->(crc);
        _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_BOI_SOT;

        if(_connection->interface()->send()){
            _state = ULSBUST_BOI_TRANSMIT_F;
        }else{
            // Error interface buffer full try to resend data later
            _state = ULSBUST_BOI_TRANSMIT_START_REPEAT;
        }
    }
    return false;
}
//===================================================
bool ULSBusTransaction::rwoiTransmitStart(){
    _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_connection->interface()->txBufInstance.buf);
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
        pxPack->rwoi_sft.cmd = ULSBUS_RWOI_SFT;
        pxPack->rwoi_sft.self_id = _self_id;
        pxPack->rwoi_sft.remote_id = _remote_id;
        pxPack->rwoi_sft.obj_id = _buf->id();
        if(_buf->getData(_connection->maxFrameSize(),0,pxPack->rwoi_sft.data,_buf->size())){ // If buffer ready
            _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_SFT + _buf->size();
            if(_connection->interface()->send())
            {
                _state = ULSBUST_RWOI_TRANSMIT_COMPLITE_WAIT_ACK;
                _timeout = ULSBUS_TIMEOUT; // keep active
            }else{
                // Error interface buffer full repeat later
                _state = ULSBUST_RWOI_TRANSMIT_START_REPEAT;
            }
        }
    }else{
        // Prepare for transmittion frames.
        initFrames();
        _cmd = ULSBUS_RWOI_SOT;
        // Transmit data SOT
        pxPack->rwoi_sot.cmd = ULSBUS_RWOI_SOT;
        pxPack->rwoi_sot.self_id = _self_id;
        pxPack->rwoi_sot.remote_id = _remote_id;
        pxPack->rwoi_sot.obj_id = _buf->id();
        pxPack->rwoi_sot.frames = _frames;
        pxPack->rwoi_sot.frame_size = _frameSize;
        pxPack->rwoi_sot.size = _buf->size();
        pxPack->rwoi_sot.crc = 0;//_buf->(crc);
        _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_SOT;

        if(_connection->interface()->send()){
            _state = ULSBUST_RWOI_TRANSMIT_F;
        }else{
            // Error interface buffer full repeat later
            _state = ULSBUST_RWOI_TRANSMIT_START_REPEAT;
        }
    }
    return false;
}
//===================================================
bool ULSBusTransaction::aoiTransmitStart(){
    _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_connection->interface()->txBufInstance.buf);

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
        pxPack->aoi_sft.cmd = ULSBUS_AOI_SFT;
        pxPack->aoi_sft.self_id = _self_id;
        pxPack->aoi_sft.remote_id = _remote_id;
        pxPack->aoi_sft.obj_id = _buf->id();

        if(_buf->getData(_connection->maxFrameSize(),0,pxPack->aoi_sft.data,_buf->size())){ // If buffer ready
            _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_AOI_SFT + _buf->size();
            if(_connection->interface()->send())
            {
                _state = ULSBUST_AOI_TRANSMIT_COMPLITE_WAIT_ACK;
                _timeout = ULSBUS_TIMEOUT; // keep active
            }else{
                // Error interface buffer full repeat later
                _state = ULSBUST_AOI_TRANSMIT_START_REPEAT;
            }
        }else{
            //int i =10;
            _state = ULSBUST_AOI_TRANSMIT_START_REPEAT;
            return false; // Buffer Not ready
        }
    }else{
        // Prepare for transmittion frames.
        initFrames();
        _cmd = ULSBUS_AOI_SOT;
        // Transmit data SOT
        pxPack->aoi_sot.cmd = ULSBUS_AOI_SOT;
        pxPack->aoi_sot.self_id = _self_id;
        pxPack->aoi_sot.remote_id = _remote_id;
        pxPack->aoi_sot.obj_id = _buf->id();
        pxPack->aoi_sot.frames = _frames;
        pxPack->aoi_sot.frame_size = _frameSize;
        pxPack->aoi_sot.size = _buf->size();
        pxPack->aoi_sot.crc = 0;//_buf->(crc);
        _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_AOI_SOT;

        if(_connection->interface()->send()){
            _state = ULSBUST_AOI_TRANSMIT_F;
            _timeout = ULSBUS_TIMEOUT; // keep active
        }else{
            // Error interface buffer full repeat later
            _state = ULSBUST_AOI_TRANSMIT_START_REPEAT;
        }
    }
    return false;
}
//================================================================
//================================================================
bool ULSBusTransaction::processPacket()
{


    if(_state == ULSBUST_EMPTY)return false;
    if(!_connection)return false;
    _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_connection->interface()->rxBufInstance.buf);

    switch(_state){
    //================================================================
    // BOI Packet processing
    case ULSBUST_BOI_TRANSMIT_START:{
        _timeout = ULSBUS_TIMEOUT; // keep active
        return boiTransmitStart();
    }
        break;
    case ULSBUST_BOI_RECEIVE_START:{
        _cmd = ULSBUS_BOI_SOT;
        _frames = pxPack->boi_sot.frames;
        _frameSize = pxPack->boi_sot.frame_size;
        _self_id = pxPack->boi_sot.self_id;
        _remote_id = 0;
        _frameLastSize = pxPack->boi_sot.size % _frameSize;
        _state = ULSBUST_BOI_RECEIVE_F;
    }
        break;
    case ULSBUST_BOI_RECEIVE_F:{
        uint8_t frameNum = pxPack->boi_f.frameNum;
        if(frameNum == _frames)
        {
            _buf->setData(_frameSize,pxPack->boi_f.frameNum,pxPack->boi_f.data,_frameLastSize);
        }else{
            _buf->setData(_frameSize,pxPack->boi_f.frameNum,pxPack->boi_f.data,_frameSize);
        }
    }
        break;
        //================================================================
        // RWOI Packet processing
    case ULSBUST_RWOI_TRANSMIT_START:{
        _timeout = ULSBUS_TIMEOUT; // keep active
        rwoiTransmitStart();
    }
        break;
    case ULSBUST_RWOI_TRANSMIT_COMPLITE_WAIT_ACK:{
        close(); // Anyway close connection.
        uint8_t ack = (pxPack->ack.ackcmd >> 5);
        if(ack == ULSBUS_ACK_COMPLITE){
            return true;
        };
        return false; // ACK with error received.
    }
        break;
    case ULSBUST_RWOI_RECEIVE_START:{
        _cmd = ULSBUS_RWOI_SOT;
        _frames = pxPack->rwoi_sot.frames;
        _frameSize = pxPack->rwoi_sot.frame_size;
        _self_id = pxPack->rwoi_sot.self_id;
        _remote_id = pxPack->rwoi_sot.remote_id;
        _frameLastSize = pxPack->rwoi_sot.size % _frameSize;
        if(_frameLastSize){
            _frames++;
        }else{
            _frameLastSize = _frameSize;
        }
        _state = ULSBUST_RWOI_RECEIVE_F;
    }
        break;
    case ULSBUST_RWOI_RECEIVE_F:{

        uint8_t frameNum = pxPack->boi_f.frameNum;
        if(frameNum == (_frames-1))
        {
            _buf->setData(_frameSize,pxPack->rwoi_f.frameNum,pxPack->rwoi_f.data,_frameLastSize);
        }else{
            _buf->setData(_frameSize,pxPack->rwoi_f.frameNum,pxPack->rwoi_f.data,_frameSize);
        }
    }
        break;
        //================================================================
        // AOI Packet processing
    case ULSBUST_AOI_TRANSMIT_START:{

        _timeout = ULSBUS_TIMEOUT; // keep active
        aoiTransmitStart();
    }
        break;
    case ULSBUST_AOI_TRANSMIT_COMPLITE_WAIT_ACK:{
        // Do nothing
        return true; // ACK with error received.
    }
        break;
    case ULSBUST_AOI_RECEIVE_START:{
        _cmd = ULSBUS_AOI_SOT;
        _frames = pxPack->aoi_sot.frames;
        _frameSize = pxPack->aoi_sot.frame_size;
        _self_id = pxPack->aoi_sot.self_id;
        _remote_id = pxPack->aoi_sot.remote_id;
        _frameLastSize = pxPack->aoi_sot.size % _frameSize;
        if(_frameLastSize){
            _frames++;
        }else{
            _frameLastSize = _frameSize;
        }
        _state = ULSBUST_AOI_RECEIVE_F;
    }
        break;
    case ULSBUST_AOI_RECEIVE_F:{

        _frameNum = pxPack->aoi_f.frameNum;
        if(_frameNum == (_frames-1))
        {
            _buf->setData(_frameSize,pxPack->aoi_f.frameNum,pxPack->aoi_f.data,_frameLastSize);

        }else{
            _buf->setData(_frameSize,pxPack->aoi_f.frameNum,pxPack->aoi_f.data,_frameSize);
        }
    }
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

    if(_timeout>0)
    {
        _timeout--;
        if(_timeout == 0){
            close(); // Close transaction by timeout
            return false;
        }
    }

    if(_state == ULSBUST_EMPTY)return false;
    if(_connection == __null)return false;
    _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_connection->interface()->txBufInstance.buf);

    switch(_state){
    //================================================================
    // BOI Task
    case ULSBUST_BOI_TRANSMIT_START_REPEAT:{
        boiTransmitStart(); // Try to start transmittion until timeout
    }
        break;
    case ULSBUST_BOI_TRANSMIT_F:{
        pxPack->boi_f.cmd = ULSBUS_BOI_F;
        pxPack->boi_f.frameNum = _frameNum;
        pxPack->boi_f.self_id = _self_id;
        pxPack->boi_f.zero = 0;
        if(_frameNum < _frames){
            if(!_buf->getData(_frameSize,_frameNum,pxPack->boi_f.data,_frameSize))return true; // Exit if no data aviable;
            _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_BOI_F +  _frameSize;
            if(_connection->interface()->send()){
                // If send ok prepare to new send
                _frameNum++;
                _timeout = ULSBUS_TIMEOUT; // keep active
            }
        }else{ //Last frame sending.
            if(!_buf->getData(_frameSize,_frameNum,pxPack->boi_f.data,_frameLastSize))return true; // Exit if no data aviable;
            _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_BOI_F +  _frameLastSize;
            if(_connection->interface()->send()){
                close(); // Close transaction no need to wait answer or RLF.
            }
        }

    }
        break;
    case ULSBUST_BOI_RECEIVE_F:{

        if(_buf->isBufferComlite())
        {
            // No ack for BIO messages.
            _state = ULSBUST_RECEIVE_BOI_COMPLITE;
        }else{
            // Check buffer if frame missed send request for lost frame
            if(_timeout == ULSBUS_TIMEOUT/2){
                // request lost frames not supported in BOI.
            }
        }
    }
        break;
    case ULSBUST_RECEIVE_BOI_COMPLITE:{
        // Copy data to Library if we found
        ULSBusObjectBase *obj =  _library->getObject(0x0,_self_id,_buf->id()); // Looking for object in library
        if(obj){ // check if we found object
            if(obj->size() == _buf->size()){ // check size match
                obj->setData(pxPack->boi_sft.data);
            }else {
                // TODO: Error here it not be, mean dictionary not consistent.
            }
        }
        close(); // All done. Close Transaction.
    }
        break;
        //================================================================
        // RWOI Task
    case ULSBUST_RWOI_TRANSMIT_START_REPEAT:{
        rwoiTransmitStart(); // Try to start transmittion until timeout
    }
        break;
    case ULSBUST_RWOI_TRANSMIT_F:{
        pxPack->rwoi_f.cmd = ULSBUS_RWOI_F;
        pxPack->rwoi_f.frameNum = _frameNum;
        pxPack->rwoi_f.self_id = _self_id;
        pxPack->rwoi_f.remote_id = _remote_id;
        if(_frameNum >= _frames)return false; // WTF? all frames already transmitted

        uint32_t size = (_frameNum  ==  (_frames-1))?_frameLastSize:_frameSize; // set size if it las frame or not

        if(!_buf->getData(_frameSize,_frameNum,pxPack->rwoi_f.data,size))return true; // Exit if no data aviable;
        _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_F +  size;
        if(_connection->interface()->send())
        {
            _frameNum++;
            _timeout = ULSBUS_TIMEOUT; // keep active

            if(_frameNum == _frames){// Last frame was sended
                _state = ULSBUST_RWOI_TRANSMIT_COMPLITE_WAIT_ACK;
            }
            return true;
        }else{
            // IDLE
            return false; // It will try to resend frame on next Task
        }
    }
        break;
    case ULSBUST_RWOI_RECEIVE_F:{

        if(_buf->isBufferComlite())
        {
            // No ack for BIO messages
            _state = ULSBUST_RWOI_RECEIVE_COMPLITE;

        }else{
            // Check buffer if frame missed send request for lost frame
            if(_timeout == ULSBUS_TIMEOUT/2){
                // request lost frames
            }
        }
    }
        break;

    case ULSBUST_RWOI_RECEIVE_COMPLITE:{
        // Copy data to Library if we found
        ULSBusObjectBase *obj =  _library->getObject(_remote_id,0,_buf->id()); // Looking for object in library
        if(obj){ // check if we found object
            if(obj->size() == _buf->size()){ // check size match
                obj->setData(_buf->pxBuf());
                // TODO: Send ACK to transmitter;
                _connection->sendAck(ULSBUS_ACK_COMPLITE,_cmd,_self_id,_remote_id);

            }else {
                // TODO: Error here it not be, mean dictionary not consistent;
                _connection->sendAck(ULSBUS_ACK_OBJECT_SIZE_MISMUTCH,_cmd,_self_id,_remote_id);
            }
        }else{
            _connection->sendAck(ULSBUS_ACK_OBJECT_NOTFOUND,_cmd,_self_id,_remote_id);
        }
        close(); // Buffer ok close connection
    }
        break;
        //================================================================
        // AOI Task
    case ULSBUST_AOI_TRANSMIT_START_REPEAT:{
        aoiTransmitStart(); // Try to start transmittion until timeout
    }
        break;
    case ULSBUST_AOI_TRANSMIT_F:{
        pxPack->aoi_f.cmd = ULSBUS_AOI_F;
        pxPack->aoi_f.frameNum = _frameNum;
        pxPack->aoi_f.self_id = _self_id;
        pxPack->aoi_f.remote_id = _remote_id;

        if(_frameNum >= _frames)return false; // WTF? all frames already transmitted

        uint32_t size = (_frameNum  ==  (_frames-1))?_frameLastSize:_frameSize; // set size if it las frame or not

        if(!_buf->getData(_frameSize,_frameNum,pxPack->aoi_f.data,size))
        {
            int i=10;
            while(i--);
            return true; // Exit if no data aviable;
        }
        _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_AOI_F +  size;
        if(_connection->interface()->send())
        {
            _frameNum++;
            _timeout = ULSBUS_TIMEOUT; // keep active
            if(_frameNum == _frames){
                _state = ULSBUST_AOI_TRANSMIT_COMPLITE_WAIT_ACK;
            }
        }else{
            // IDLE
            return false; // It will try to resend frame on next Task
        }
    }
        break;
    case ULSBUST_AOI_RECEIVE_F:{

        if(_buf->isBufferComlite())
        {
            _state = ULSBUST_AOI_RECEIVE_COMPLITE;
        }else{
            // Check buffer if frame missed send request for lost frame
            if(_timeout == ULSBUS_TIMEOUT/2){
                // request lost frames
            }
        }
    }
        break;

    case ULSBUST_AOI_RECEIVE_COMPLITE:{
        // Copy data to Library if we found
        ULSBusObjectBase *obj =  _library->getObject(_remote_id,0,_buf->id()); // Looking for object in library
        if(obj){ // check if we found object
            if(obj->size() == _buf->size()){ // check size match
                obj->setData(pxPack->boi_sft.data);
                 _connection->sendAck(ULSBUS_ACK_COMPLITE,_cmd,_self_id,_remote_id);
            }else {
                _connection->sendAck(ULSBUS_ACK_OBJECT_SIZE_MISMUTCH,_cmd,_self_id,_remote_id);
            }
        }else {
            _connection->sendAck(ULSBUS_ACK_OBJECT_NOTFOUND,_cmd,_self_id,_remote_id);
        }
        close(); // Buffer ok close connection
    }
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
