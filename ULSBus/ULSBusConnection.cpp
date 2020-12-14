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

#include "ULSBusConnection.h"

ULSBusConnection::ULSBusConnection(const char* name,uint8_t did,uint8_t cid):
    //ULSListItem(),
    ULSBusInterface(name,did),
    _cid(cid)
{
    cnRxPacket = (_cn_packet*)ifRxBuf;
    cnTxPacket = (_cn_packet*)ifTxBuf;
    ifRxLen = 0;
};

void ULSBusConnection::deviceConnected(uint8_t id)
{
#ifdef ULS_DEBUG
    uDebug("%s: Device Connected 0x%.2X ",_name,id);
#else
    (void) id;
#endif
}
void ULSBusConnection::deviceDisconnected(uint8_t id)
{
#ifdef ULS_DEBUG
    uDebug("%s: Device Disconnected 0x%.2X ",_name,id);
#else
    (void) id;
#endif

}

_io_op_rezult ULSBusConnection::cnSendDiscovery()
{
    cnTxPacket->ridx = 0;
    ifTxLen = 1;
    return ifSend();
}
_io_op_rezult ULSBusConnection::cnForward(ULSBusConnection *sc)
{
    uint8_t *pxDsn = cnTxPacket->pld;
    uint8_t *pxSrc = sc->cnTxPacket->pld;
    uint8_t ridx = sc->cnTxPacket->ridx;
    uint32_t len = sc->ifTxLen;

    for(int i=0;i < ridx;i++){
        *pxDsn++ =  *pxSrc++;
        if(len == 0)return IO_ERROR;
        len--;
    }
    *pxDsn++= cnrid(sc->cid()); // Add Route ID to list
    cnTxPacket->ridx = ridx + 1; // Add route count
    ifTxLen = sc->ifRxLen + 1; // Add route count
    memcpy(pxDsn,pxSrc,len);
    return ifSend();
}

_io_op_rezult ULSBusConnection::cnReceive()
{
    _io_op_rezult rez = ifReceive();
    if(rez != IO_OK) return rez;
    if(ifRxLen < 1) return IO_ERROR;
    ifRxLen -= 1;

    ifRxLen = 0;
    return IO_OK;
}
