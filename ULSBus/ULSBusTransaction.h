#ifndef ULSBUSTRANSACTION_H
#define ULSBUSTRANSACTION_H
#include "IfBase.h"
#include "ULSBusConnection.h"
#include "ULSBusObject.h"
#include "ULSBusObjectBuffer.h"


class ULSBusTransaction{
public:
    ULSBusTransaction(){close();};
    void library(ULSBusObjectsLibrary    *library)
    {
        _library = library;
    }

    bool open(ULSBusConnection* connection,uint8_t selfId,uint8_t remoteId)
    {
        if(_state != ULSBUST_EMPTY) return false;
        _state = ULSBUST_BUSY;
        _connection = connection;
        _selfId = selfId;
        _remoteId = remoteId;
        _buf = __null;
        _timeout = ULSBUS_TIMEOUT;
        return true;
    }
    bool open(ULSBusConnection* connection,ULSBusConnection* connectionGate,uint8_t selfId,uint8_t remoteId)
    {
        if(_state != ULSBUST_EMPTY) return false;
        _connectionGate = connectionGate;
        return open(connection,selfId,remoteId);
    }
    void close()
    {
        _state = ULSBUST_EMPTY;
        _frames = 0;
        _frameSize = 0;
        _frameLastSize = 0;
        _frameNum = 0;
        disconnectBuffer();
        _connection = __null;
        _connectionGate = __null;
    }
    void close(uint8_t cmd,uint8_t remoteID)
    {
        if((cmd == _cmd)&&(remoteID == _remoteId)) close();
    }

    void connectBuffer(ULSBusObjectBuffer* buf)
    {
        _buf = buf;
        buf->connect();
    }
    void disconnectBuffer()
    {
        if(!_buf)return;
        _buf->disconnect();
        _buf = __null;
    }
    bool check(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id,_ulsbus_transaction_state state)
    {
        if((connection == _connection)&&
                (self_id == _selfId)&&
                (remote_id == _remoteId)&&
                (state == _state)) return true;
        return false;
    }
    //===================================================
    bool boiTransmitStart(){
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
            pxPack->boi_sft.cmd = ULSBUS_BOI_SFT;
            pxPack->boi_sft.self_id = _selfId;
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
            _frameSize = _connection->maxFrameSize();
            _frameLastSize = _buf->size()%_frameSize;
            _frames = _buf->size()/_frameSize;
            _frameNum = 0;
            if(_frameLastSize == 0){
                _frames--;
            }
            // Transmit SOT.
            pxPack->boi_sot.cmd = ULSBUS_BOI_SOT;
            pxPack->boi_sot.self_id = _selfId;
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
    bool rwoiTransmitStart(){
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
            pxPack->rwoi_sft.cmd = ULSBUS_RWOI_SFT;
            pxPack->rwoi_sft.self_id = _selfId;
            pxPack->rwoi_sft.remote_id = _remoteId;
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
            _frameSize = _connection->maxFrameSize();
            _frameLastSize = _buf->size()%_frameSize;
            _frames = _buf->size()/_frameSize;
            _frameNum = 0;
            if(_frameLastSize == 0){
                _frames--;
            }
            // Transmit data SOT
            pxPack->rwoi_sot.cmd = ULSBUS_RWOI_SOT;
            pxPack->rwoi_sot.self_id = _selfId;
            pxPack->rwoi_sot.remote_id = _remoteId;
            pxPack->rwoi_sot.obj_id = _buf->id();
            pxPack->rwoi_sot.frames = _frames;
            pxPack->rwoi_sot.frame_size = _frameSize;
            pxPack->rwoi_sot.size = _buf->size();
            pxPack->rwoi_sot.crc = 0;//_buf->(crc);
            _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_SOT;

            if(_connection->interface()->send()){
                _state = ULSBUST_RWOI_TRANSMIT_SOT_WAIT_ACK;
            }else{
                // Error interface buffer full repeat later
                _state = ULSBUST_RWOI_TRANSMIT_START_REPEAT;
            }
        }
        return false;
    }
    //===================================================
    bool aoiTransmitStart(){
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
            pxPack->aoi_sft.cmd = ULSBUS_AOI_SFT;
            pxPack->aoi_sft.self_id = _selfId;
            pxPack->aoi_sft.remote_id = _remoteId;
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
            }
        }else{
            // Prepare for transmittion frames.
            _frameSize = _connection->maxFrameSize();
            _frameLastSize = _buf->size()%_frameSize;
            _frames = _buf->size()/_frameSize;
            _frameNum = 0;
            if(_frameLastSize == 0){
                _frames--;
            }
            // Transmit data SOT
            pxPack->aoi_sot.cmd = ULSBUS_AOI_SOT;
            pxPack->aoi_sot.self_id = _selfId;
            pxPack->aoi_sot.remote_id = _remoteId;
            pxPack->aoi_sot.obj_id = _buf->id();
            pxPack->aoi_sot.frames = _frames;
            pxPack->aoi_sot.frame_size = _frameSize;
            pxPack->aoi_sot.size = _buf->size();
            pxPack->aoi_sot.crc = 0;//_buf->(crc);
            _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_AOI_SOT;

            if(_connection->interface()->send()){
                _state = ULSBUST_AOI_TRANSMIT_SOT_WAIT_ACK;
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
    bool processPacket()
    {


        if(_state == ULSBUST_EMPTY)return false;
        if(!_connection)return false;
        _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_connection->interface()->txBufInstance.buf);

        switch(_state){
        //================================================================
        // BOI Packet processing
        case ULSBUST_BOI_TRANSMIT_START:{
            _timeout = ULSBUS_TIMEOUT; // keep active
            return boiTransmitStart();
        }
            break;
        case ULSBUST_BOI_RECEIVE_START:{
            _frames = pxPack->boi_sot.frames;
            _frameSize = pxPack->boi_sot.frame_size;
            _selfId = pxPack->boi_sot.self_id;
            _remoteId = 0;
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
            if((pxPack->ack.ackcmd == ULSBUS_ACK_RWOI_SFT_OK)||
                    (pxPack->ack.ackcmd == ULSBUS_ACK_RWOI_SOT_COMPLITE)){
                return true;
            };
            return false; // ACK with error received.
        }
            break;
        case ULSBUST_RWOI_TRANSMIT_SOT_WAIT_ACK:{
            if(pxPack->ack.ackcmd == ULSBUS_ACK_RWOI_SOT_OK){
                _state = ULSBUST_RWOI_TRANSMIT_F;
                return true;
            };
            close(); // ACK with error received.
            return false;
        }
            break;
        case ULSBUST_RWOI_RECEIVE_START:{
            _frames = pxPack->rwoi_sot.frames;
            _frameSize = pxPack->rwoi_sot.frame_size;
            _selfId = pxPack->rwoi_sot.self_id;
            _remoteId = pxPack->rwoi_sot.remote_id;
            _frameLastSize = pxPack->boi_sot.size % _frameSize;
            _state = ULSBUST_RWOI_RECEIVE_F;
        }
            break;
        case ULSBUST_RWOI_RECEIVE_F:{

            uint8_t frameNum = pxPack->boi_f.frameNum;
            if(frameNum == _frames)
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
            close(); // Anyway close connection.
            if((pxPack->ack.ackcmd == ULSBUS_ACK_RROI_OK)||
                    (pxPack->ack.ackcmd == ULSBUS_ACK_RWOI_SOT_COMPLITE)){
                return true;
            };
            return false; // ACK with error received.
        }
            break;
        case ULSBUST_AOI_TRANSMIT_SOT_WAIT_ACK:{

        }
            break;
        case ULSBUST_AOI_RECEIVE_F:{

            uint8_t frameNum = pxPack->boi_f.frameNum;
            if(frameNum == _frames)
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
    bool task() // calls every ms
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
        if(_connection)return false;
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
            pxPack->boi_f.self_id = _selfId;
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
            ULSBusObjectBase *obj =  _library->getObject(0x0,_selfId,_buf->id()); // Looking for object in library
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
            pxPack->rwoi_f.self_id = _selfId;
            pxPack->rwoi_f.remote_id = _remoteId;
            if(_frameNum < _frames){ // If Not last frame.
                if(!_buf->getData(_frameSize,_frameNum,pxPack->rwoi_f.data,_frameSize))return true; // Exit if no data aviable;
                _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_F +  _frameSize;
                if(_connection->interface()->send())
                {
                    _frameNum++;
                    _timeout = ULSBUS_TIMEOUT; // keep active
                    return true;
                }else{
                    return false; // It will try to resend frame on next Task
                }
            }else{ // If last frame.
                if(!_buf->getData(_frameSize,_frameNum,pxPack->rwoi_f.data,_frameLastSize))return true; // Exit if no data aviable;
                _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_F +  _frameLastSize;
                if(_connection->interface()->send()){

                    _state = ULSBUST_RWOI_TRANSMIT_COMPLITE_WAIT_ACK;
                    _timeout = ULSBUS_TIMEOUT; // keep active
                    return true;
                }else{
                    return false;// It will try to resend frame on next Task
                }
            }
        }
            break;

            //        case ULSBUST_RWOI_TRANSMIT_WAIT_ACK:{
            //            //ULSBUST_RWOI_TRANSMIT_WAIT_ACK
            //            // Wait ack or timeout
            //        }

            break;
        case ULSBUST_RWOI_RECEIVE_F:{

            if(_buf->isBufferComlite())
            {
                // No ack for BIO messages
                _state = ULSBUST_RECEIVE_BOI_COMPLITE;

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
            ULSBusObjectBase *obj =  _library->getObject(_remoteId,0,_buf->id()); // Looking for object in library
            if(obj){ // check if we found object
                if(obj->size() == _buf->size()){ // check size match
                    obj->setData(pxPack->boi_sft.data);
                    // TODO: Send ACK to transmitter;

                }else {
                    // TODO: Error here it not be, mean dictionary not consistent;
                }
            }
            close(); // Buffer ok close connection
        }
            //================================================================
            // AOI Task
        case ULSBUST_AOI_TRANSMIT_START_REPEAT:{
            aoiTransmitStart(); // Try to start transmittion until timeout
        }
            break;
        case ULSBUST_AOI_TRANSMIT_F:{
            pxPack->aoi_f.cmd = ULSBUS_AOI_F;
            pxPack->aoi_f.frameNum = _frameNum;
            pxPack->aoi_f.self_id = _selfId;
            pxPack->aoi_f.remote_id = _remoteId;
            if(_frameNum < _frames){

                if(!_buf->getData(_frameSize,_frameNum,pxPack->aoi_f.data,_frameSize))return true; // Exit if no data aviable;
                _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_F +  _frameSize;
                if(_connection->interface()->send())
                {
                    _frameNum++;
                    _timeout = ULSBUS_TIMEOUT; // keep active
                }

            }else{ //read last frame;

                if(!_buf->getData(_frameSize,_frameNum,pxPack->aoi_f.data,_frameLastSize))return true; // Exit if no data aviable;
                _connection->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RWOI_F +  _frameLastSize;
                if(_connection->interface()->send()){

                    _state = ULSBUST_AOI_TRANSMIT_COMPLITE_WAIT_ACK;
                    _timeout = ULSBUS_TIMEOUT; // keep active
                }
            }

        }
            break;

            //        case ULSBUST_AOI_TRANSMIT_WAIT_ACK:{
            //            //ULSBUST_AOI_TRANSMIT_WAIT_ACK
            //            // Wait ack or timeout
            //        }

            break;
        case ULSBUST_AOI_RECEIVE_F:{

            if(_buf->isBufferComlite())
            {
                // No ack for BIO messages
                _state = ULSBUST_RECEIVE_BOI_COMPLITE;

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
            ULSBusObjectBase *obj =  _library->getObject(_remoteId,0,_buf->id()); // Looking for object in library
            if(obj){ // check if we found object
                if(obj->size() == _buf->size()){ // check size match
                    obj->setData(pxPack->boi_sft.data);
                    // TODO: Send ACK to transmitter;

                }else {
                    // TODO: Error here it not be, mean dictionary not consistent;
                }
            }
            close(); // Buffer ok close connection
        }

        default:
            break;
        }
        return true;
    }


    void state(_ulsbus_transaction_state state)
    {
        _state = state;
    }
    _ulsbus_transaction_state state()
    {
        return _state;
    }

    ULSBusObjectBuffer* buffer(){return _buf;};
private:
    ULSBusObjectBuffer* _buf;
    ULSBusConnection *_connection; // Pointer to main Connection
    ULSBusConnection *_connectionGate; // Pointer to gate connection Connection

    ULSBusObjectsLibrary    *_library;
    _ulsbus_transaction_state _state;
    uint32_t _timeout;
    uint8_t  _cmd;
    uint8_t  _selfId;
    uint8_t  _remoteId;
    uint8_t  _frames;
    uint8_t  _frameNum;
    uint8_t  _frameSize;
    uint8_t  _frameLastSize;
    //    uint16_t _crc;

};
template<int SIZE>
class ULSBusTransactionsList{
public:
    ULSBusTransactionsList()
    {
        for(int i=0;i<SIZE;i++)
        {
            _transaction[i].close();
        }
    };
    void library(ULSBusObjectsLibrary    *library){
        for(int i=0;i<SIZE;i++)
        {
            _transaction[i].library(library);
        }
    }
    void task(){
        for(int i=0;i<SIZE;i++)
        {
            _transaction[i].task();
        }
    }
    ULSBusTransaction* open(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id)
    {
        for(int i=0;i<SIZE;i++)
        {
            if(_transaction[i].open(connection,self_id,remote_id) )return &_transaction[i];
        }
        return __null;
    }

    ULSBusTransaction* open(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id,ULSBusObjectBuffer* buf,_ulsbus_transaction_state state)
    {
        for(int i=0;i<SIZE;i++)
        {
            if(_transaction[i].open(connection,self_id,remote_id) ){
                if(buf){
                    _transaction[i].connectBuffer(buf);
                }
                _transaction[i].state(state);
                _transaction[i].processPacket();
                return &_transaction[i];
            }
        }
        return __null;
    }

    ULSBusTransaction* open(ULSBusConnection* connection,ULSBusConnection* connectionGate,uint8_t self_id,uint8_t remote_id,ULSBusObjectBuffer* buf,_ulsbus_transaction_state state)
    {
        for(int i=0;i<SIZE;i++)
        {
            if(_transaction[i].open(connection,connectionGate,self_id,remote_id) ){
                if(buf){
                    _transaction[i].connectBuffer(buf);
                }
                _transaction[i].state(state);
                _transaction[i].processPacket();
                return &_transaction[i];
            }
        }
        return __null;
    }
    ULSBusTransaction* find(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id,_ulsbus_transaction_state state)
    {
        for(int i=0;i<SIZE;i++)
        {
            if(_transaction[i].check(connection,self_id,remote_id,state))return &_transaction[i];
        }
        return __null;
    }
private:
    ULSBusTransaction _transaction[SIZE];
};
#endif // ULSBUSTRANSACTION_H
