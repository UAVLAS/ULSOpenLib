#include "ULSSerial.h"


ULSSerial::ULSSerial(_io_fifo_u8 *rxFifo,_io_fifo_u8 *txFifo)
{
    _str[0] = 0;
    _crc = 0;
    _len = 0;
    _rxFifo = rxFifo;
    _txFifo = txFifo;
    _isOpened = false;
    _txCplt = true;
    _mode = SERIAL_MODE_RAW;
    _crcerrors = 0;
    _packeterrors = 0;
    _stage = 0;
    _escStarted = false;
}

uint32_t ULSSerial::write(uint8_t *buf,uint32_t size)
{
    switch (_mode) {
    case SERIAL_MODE_RAW:
        size = _txFifo->push(buf,size);
        break;
    case SERIAL_MODE_ESC:
        size = writeEsc(buf,size);
        break;
    case SERIAL_MODE_COBS:
        size = writeCobs(buf,size);
        break;
    }
    transmitterUpdate();
    return size;
}

uint32_t ULSSerial::writeCobs(uint8_t *buf,uint32_t size)
{
    uint8_t *pxSlip;
    uint8_t code;
    uint32_t len = size;
    uint16_t crc = 0;
    // Start block
    pxSlip = _txFifo->cobs();
    code = 1;
    _txFifo->pushcobs(0);
    
    while(len > 0U){
        if (code != 0xFF) {
            len--;
            uint8_t c = *buf++;
            crc = GetCrcByte(crc,c);
            if (c != 0) {
                _txFifo->pushcobs(c);
                code++;
                continue;
            }
        }
        *pxSlip = code; // finish block
        // Start block
        pxSlip = _txFifo->cobs();
        code = 1;
        _txFifo->pushcobs(0);
    }
    // send CRC
    len = 2;
    buf = (uint8_t*)&crc;
    while(len > 0U)
    {
        len--;
        if (code != 0xFF) {
            uint8_t c = *buf++;
            if (c != 0) {
                _txFifo->pushcobs(c);
                code++;
                continue;
            }
        }
        *pxSlip = code; // finish block
        // Start block
        pxSlip = _txFifo->cobs();
        code = 1;
        _txFifo->pushcobs(0);
    }
    
    *pxSlip = code; // finish block
    _txFifo->pushcobs(0);
    _txFifo->releasecobs();
    return size;
}
uint32_t ULSSerial::writeEsc(uint8_t *buf,uint32_t size)
{
    uint16_t crc = 0;
    uint32_t N = size;
    
    if (size == 0)return 0;
    
    if(!_txFifo->push(0x55))return 0;
    if(!_txFifo->push(0x01))return 0;
    while (N) {
        if(!_txFifo->push(*buf))return 0;
        crc = GetCrcByte(crc,*buf);
        if (*buf==0x55) {
            if(!_txFifo->push(0x02))return 0;
        }
        buf++;
        N--;
    }
    
    if(!_txFifo->push(crc&0xFF))return 0;
    if ((crc&0xFF)==0x55) {
        if(!_txFifo->push(0x02))return 0;
    }
    
    if(!_txFifo->push((crc>>8)&0xFF))return 0;
    if (((crc>>8)&0xFF)==0x55) {
        if(!_txFifo->push(0x02))return 0;
    }
    
    if(!_txFifo->push(0x55))return 0;
    if(!_txFifo->push(0x03))return 0;
    
    return size;
}

//==============================================================================
uint32_t ULSSerial::read(uint8_t *buf,uint32_t sizelimit)
{
    uint32_t size = 0;
    switch (_mode) {
    case SERIAL_MODE_RAW:
        size = _rxFifo->pull(buf,sizelimit);
        break;
    case SERIAL_MODE_ESC:
        size = readEsc(buf,sizelimit);
        break;
    case SERIAL_MODE_COBS:
        size = readCobs(buf,sizelimit);
        break;
    }
    receiverUpdate();
    return size;
}
//==============================================================================
uint32_t ULSSerial::readCobs(uint8_t *buf,uint32_t sizelimit)
{
    bool packInBuf = false;
    unsigned char v=0;
    uint32_t size = 0;
    uint16_t crc = 0;


    while (_rxFifo->seek(&v)){
        switch(_stage)
        {
        case 0: // Start Of packet
            _code = _copy = v;
            _stage = 1;
            _len = 0;
            break;
        case 1:
            // Analayzing packet
            _copy--;
            if(_copy != 0){
                _len++;
                crc = GetCrcByte(crc,v);
            }else {
                if(v == 0){
                    _stage = 0;
                    if((crc != 0)||(_len<2)){
#ifdef ULS_DEBUG
                        uDebug("COBS Received packet CRC Mismatch");
#endif
                        _rxFifo->flush_to_seeker();
                        return 0;
                    }
                    // CRC Ok buffer len calculated;
                    packInBuf = true;
                }else{
                    if(_code != 0xFF){
                        _len++;
                        crc = GetCrcByte(crc,0);
                    }
                }
                _copy = _code = v;
            }
            break;
        }
        if(packInBuf)break;
    }


    if(!packInBuf)return 0;
    _len -= 2; // Remove CRC;
    size = _len;
    if(_len > sizelimit){
        // unable to receive complite pack
        _rxFifo->flush_to_seeker();
        return 0 ;
    }

    if(!_rxFifo->pull(&v))return 0; // WTF
    if(v == 0) return 0;
    _code = _copy = v;

    while(_rxFifo->pull(&v))
    {
        _copy--;
        if (_copy != 0) {
            if(_len){*buf++ = v;
                _len--;
            }
        } else {
            if (v == 0){
                if(crc != 0){
#ifdef ULS_DEBUG
                    uDebug("COBS Received packet CRC Mismatch");
#endif
                    _rxFifo->flush_to_seeker();
                    return 0;
                }
                return size;
            }
            if (_code != 0xFF){
                if(_len){
                    *buf++ = 0;
                    _len--;
                }
                crc = GetCrcByte(crc,0);
            }
            _copy = _code = v;
        }
    }
    _rxFifo->flush_to_seeker();
    return 0;
}
//==============================================================================
uint32_t ULSSerial::readEsc(uint8_t *buf,uint32_t sizelimit)
// 0x55..0x01..DATA(0x55.0x02)..0x55..0x03
{
    receiverUpdate();
    unsigned char v=0;
    bool isEscInBuf = false;
    //    stage = 0;
    while (_rxFifo->seek(&v)) {
        switch (_stage) {
        case 0:
            if (v==0x55) {
                _stage = 1;
            }
            else {
                if(_escStarted){
                    _crc = GetCrcByte(_crc,v);
                    _len++;
                    if(_len > _rxFifo->size()){
                        //packet too big;
                        _escStarted = false;
                        _rxFifo->flush_to_seeker();
                        _crc = 0;
                        _len = 0;
                    }
                }else{
                    _rxFifo->flush_to_seeker();// Flush all data before packet
                }
            }
            break;
        case 1:
            if (v == 0x01) { //start of package fush to here
                _escStarted = true;
                _rxFifo->flush_to_seeker();
                _crc = 0;
                _len = 0;
            }
            else if (v == 0x02) {
                _crc = GetCrcByte(_crc,0x55);
                _len++;
            }
            else if (v == 0x03) {
                // End of pack reached
                _escStarted = false;
                if(_crc !=0){ // CRC error
                    _rxFifo->flush_to_seeker();// Flush all data before packet
                    _crcerrors++;
                    break;
                }else{
                    isEscInBuf = true;
                }
            }
            _stage = 0;
            break;
        }
        if (isEscInBuf) {
            break;    // Finish seek if esc in buffer
        }
    }
    
    // Substract CRC
    uint32_t crcLen = 2;
    
    // If Packet in buffer - Return packet data.
    if (isEscInBuf && (_len >= crcLen)) {
        uint32_t cnt=0,stage = 0;
        while (_rxFifo->pull(&v)) {
            if (cnt == (_len-crcLen)) {
                // All data in buf
                // Skip 2 b ytes of crc
                _rxFifo->flush_to_seeker();
                return cnt;
            }
            if (cnt == sizelimit) {
                // Error received packet tooo big
                // Flush this packet
                _rxFifo->flush_to_seeker();
                return 0;
            }
            
            // Packet in buffer
            switch (stage) {
            case 0:
                if (v==0x55) {
                    stage = 1;
                }
                else {
                    *buf=v;
                    buf++;
                    cnt++;
                }
                break;
            case 1:
                if (v == 0x02) {
                    *buf=0x55;
                    buf++;
                    cnt++;
                }
                else if (v == 0x03) {
                    // End of pack reached never come here
                    return cnt;
                }
                stage = 0;
                break;
            }
        }
    }
    
    return 0;
}
void ULSSerial::flush()
{
    _rxFifo->flush();
}
bool ULSSerial::txCplt()
{
    return _txCplt;
}
void ULSSerial::writeString(const char *format,...)
{
    va_list args;
    va_start(args, format);
    vsprintf(_str,format, args);
    va_end(args);
    write((uint8_t*)_str,(uint32_t)strlen(_str));
}

