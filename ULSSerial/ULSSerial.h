#ifndef ULSSERIAL_H
#define ULSSERIAL_H

#include "string.h"
#include <stdio.h>
#include <stdarg.h>
#include "../utils/crc16.h"
#include "../utils/io_fifo.h"

#define ULSSERIAL_STR_BUFFER_SIZE 512

typedef enum{
    SERIAL_MODE_RAW = 0,
    SERIAL_MODE_ESC = 1,
    SERIAL_MODE_COBS = 2
}_serial_mode;

class ULSSerial
{
public:
    ULSSerial(_io_fifo_u8 *rxBuf,_io_fifo_u8 *txBuf);
    uint32_t read(uint8_t *buf,uint32_t sizelimit);
    uint32_t readEsc(uint8_t *buf,uint32_t sizelimit);
    uint32_t readCobs(uint8_t *buf,uint32_t sizelimit);

    virtual void close(){};


    uint32_t write(uint8_t *buf,uint32_t size);
    uint32_t writeEsc(uint8_t *buf,uint32_t size);
    uint32_t writeCobs(uint8_t *buf,uint32_t size);

    void mode(_serial_mode mode){_mode = mode;}
    _serial_mode mode(){return _mode;}

    void setCrcByte(bool);

    void flush();

    void txCompliteCallback();
    void writeString(const char *format,...);

    bool txCplt();
    uint32_t _crcerrors;
    uint32_t _packeterrors;

protected:
    virtual void receiverUpdate(){};
    virtual void transmitterUpdate(){};
    _io_fifo_u8 *_rxFifo;
    _io_fifo_u8 *_txFifo;
    bool _txCplt;
    bool _isOpened;


private:
    _serial_mode _mode;
    uint16_t _crc;
    uint32_t _len;
    uint8_t _cobs_code;
    uint8_t _cobs_counter;
    uint32_t _stage;
    bool _escStarted;
    char _str[ULSSERIAL_STR_BUFFER_SIZE];
    bool readCobsCheck(uint32_t sizelimit);
};


#endif // ULSSERIAL_H
