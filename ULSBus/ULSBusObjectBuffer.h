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

#ifndef ULSBUSOBJECTBUFFER_H
#define ULSBUSOBJECTBUFFER_H
#include "ULSBusTypes.h"

class ULSBusObjectBuffer:public ULSListItem{
public:
    ULSBusObjectBuffer();
    void close();
    bool setData(uint8_t frame_size,uint8_t frame,uint8_t *buf, uint32_t len);
    bool setData(uint8_t *buf, uint16_t len);
    bool getData(uint8_t frame_size,uint8_t frame,uint8_t *buf, uint16_t len);
    bool isBusy();
    bool open(uint16_t id,uint16_t size);
    void connect();
    void disconnect();
    bool isBufferComlite();

public:
    uint16_t id(){return _id;}
    uint16_t size(){return _size;}
    uint8_t* pxBuf(){return _buf;}
    uint32_t frameValidMask[64];

private:
    uint8_t  _buf[2048];
    uint32_t _interfacesConnected;
    uint16_t _id;
    uint32_t _frames;
    uint16_t _size;
    uint32_t _sizeMax;
};

class ULSBusObjectBufferList:public ULSList<ULSBusObjectBuffer>
{
public:
    ULSBusObjectBufferList();

    ULSBusObjectBuffer* open(uint16_t id,uint16_t size);
private:
};

#endif // ULSBUSOBJECTBUFFER_H
