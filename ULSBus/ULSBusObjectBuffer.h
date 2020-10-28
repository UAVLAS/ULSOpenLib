#ifndef ULSBUSOBJECTBUFFER_H
#define ULSBUSOBJECTBUFFER_H
#include<inttypes.h>

class ULSBusObjectBuffer{
public:
    ULSBusObjectBuffer():
        _interfacesConnected(0),
        _id(0),
        _size(0),
        _sizeMax(2048)
    {
        close();
    }
    void close()
    {
        for(uint32_t i=0; i< 64; i++ )
            frameValidMask[i] = 0;
    }
    bool setData(uint8_t frame_size,uint8_t frame,uint8_t *buf, uint32_t len)
    {
        uint32_t start = frame*frame_size;
        if((start+len)  >= _sizeMax)return false;

        for(uint32_t i=0 ;i<len; i++)
        {
            uint32_t idx = start + i;
            uint32_t maskFrame = idx/8;
            frameValidMask[maskFrame/32] |= (1<<(idx%32));
            _buf[idx] = *buf++   ;
        }
        return true;
    }
    bool setData(uint8_t *buf, uint16_t len)
    {
        return setData(0,0,buf,len);
    }
    bool getData(uint8_t frame_size,uint8_t frame,uint8_t *buf, uint16_t len)
    {
        uint32_t start = frame*frame_size;
        if((start+len)  >= _sizeMax)return false;
        for(uint32_t i=0 ;i<len; i++)
        {
            uint32_t idx = start + i;
            uint32_t maskFrame = idx/8;
            if( ( frameValidMask[maskFrame/32] & (1<<(idx%32))) != 0){
                 *buf++ = _buf[idx];
            }else{
                return false; // data not ready;
            }
        }
        return true;
    }
    bool isBusy(){return (_interfacesConnected != 0);}
    bool open(uint16_t id,uint16_t size)
    {
        if(isBusy())return false;
        if(size > _sizeMax)return false;
        if(size == 0)return false;
        _id = id;
        _size = size;
        _frames = size/8;
        if((size%8)==0)_frames--;
        return true;
    }
    void connect()
    {
        _interfacesConnected++;
    }

    void disconnect()
    {
        if(_interfacesConnected)_interfacesConnected--;
        if(_interfacesConnected == 0) close();
    }
    bool isBufferComlite(){
        for(uint32_t i = 0 ; i < _frames; i++){
            if(!(frameValidMask[i/32]&(1<<(i%32))))return false;
        }
        return true;
    }
    //bool getLostFrame();
    uint16_t id(){return _id;}
    uint16_t size(){return _size;}

    uint32_t frameValidMask[64];

private:
    uint8_t  _buf[2048];
    uint32_t _interfacesConnected;
    uint16_t _id;
    uint32_t _frames;
    uint16_t _size;
    uint32_t _sizeMax;
};


template<int SIZE>
class ULSBusObjectBufferList
{
public:
    ULSBusObjectBufferList(){};

    ULSBusObjectBuffer* open(uint16_t id,uint16_t size)
    {
        for(int i=0 ; i < SIZE ; i++){
            if(_objectBuffer[i].open(id,size))return &_objectBuffer[i];
        }
        return __null;
    }
private:
    ULSBusObjectBuffer _objectBuffer[SIZE];
};




#endif // ULSBUSOBJECTBUFFER_H
