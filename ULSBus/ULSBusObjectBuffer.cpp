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

#include "ULSBusObjectBuffer.h"

ULSBusObjectBuffer::ULSBusObjectBuffer():ULSListItem(),
    _interfacesConnected(0),
    _id(0),
    _frames(0),
    _size(0),
    _sizeMax(2048),
    _obj(__null)
{
    for(uint32_t i=0 ;i<_sizeMax; i++)
    {
        _buf[i]=0;
    }
    close();
}
void ULSBusObjectBuffer::close()
{
    _interfacesConnected = 0;
    _obj = __null;
    for(uint32_t i=0; i< 8; i++ )
        frameValidMask[i] = 0;

}
bool ULSBusObjectBuffer::setData(uint32_t frame_size,uint32_t frame_idx,uint8_t *buf, uint32_t len)
{
    uint32_t start = frame_size * frame_idx ;
    uint32_t end = start + len;
    if(end > _sizeMax)return false;

    for(uint32_t idx = start ;idx < end; idx++){
        uint32_t maskFrame = idx/8;
        frameValidMask[maskFrame/32] |= (1<<(maskFrame%32));
        _buf[idx] = *buf++   ;
    }
    return true;
}
bool ULSBusObjectBuffer::setData(uint8_t *buf, uint32_t len)
{
    return setData(0,0,buf,len);
}
bool ULSBusObjectBuffer::getData(uint32_t frame_size,uint32_t frame_idx,uint8_t *buf, uint32_t len)
{
    uint32_t start = frame_size * frame_idx ;
    uint32_t end = start + len;
    if(end > _sizeMax)return false;

    for(uint32_t idx = start ;idx < end; idx++){
        uint32_t maskFrame = idx/8;
        if( ( frameValidMask[maskFrame/32] & (1<<(maskFrame%32))) != 0){
            *buf++ = _buf[idx];
        }else{
            int i = 10;
            while(i--);
            return false; // data not ready;
        }
    }
    return true;
}
bool ULSBusObjectBuffer::setFromObject()
{
    if(_obj == __null)return false;
    return setData(_obj->data(),_obj->size());
}
bool ULSBusObjectBuffer::setToObject()
{
    if(_obj == __null)return false;
    _obj->setData(_buf);
    _obj->state(ULSBUS_OBJECT_STATE_OK);
     return true;
}
bool ULSBusObjectBuffer::isBusy()
{
    return (_interfacesConnected != 0);
}
bool ULSBusObjectBuffer::open(uint16_t id,uint16_t size,ULSBusObjectBase *obj)
{
    if(isBusy())return false;
    if(size > _sizeMax)return false;
    if(size == 0)return false;
    _obj = obj;
    _id = id;
    _size = size;
    _frames = size/8;
    if(size%8)_frames++;
    return true;
}
void ULSBusObjectBuffer::connect()
{
    _interfacesConnected++;
}
void ULSBusObjectBuffer::disconnect()
{
    if(_interfacesConnected > 0){
        _interfacesConnected--;
    }
    if(_interfacesConnected == 0){
        close();
    }
}
bool ULSBusObjectBuffer::isBufferComlite(){
    for(uint32_t i = 0 ; i < _frames; i++){
        if(!(frameValidMask[i/32]&(1<<(i%32))))return false;
    }
    return true;
}
ULSBusObjectBufferList::ULSBusObjectBufferList():ULSList()
{

};

ULSBusObjectBuffer* ULSBusObjectBufferList::open(uint16_t id,uint16_t size,ULSBusObjectBase *obj)
{
    ULSBusObjectBuffer *px = head();
    while(px){
        if(px->open(id,size,obj))return px;
        px = forward(px);
    };
    return __null;
}



