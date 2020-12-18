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

ULSBusConnection::ULSBusConnection(ULSBusConnectionsList* connections,const char* name,uint8_t did,uint8_t cid):
    ULSListItem(),
    ULSBusInterface(name,did),
    cnclbkConnected(nullptr),
    cnclbkStatusReceived(nullptr),
    cnclbkObjReceived(nullptr),
    cnclbkObjRequested(nullptr),
    _cid(cid),
    _connections(connections)
{
    connections->add(this);
    cnRxPacket = (_cn_packet*)ifRxBuf;
    cnTxPacket = (_cn_packet*)ifTxBuf;
}

void ULSBusConnection::task(uint32_t dtms){
    cnReceive();
    ULSBusInterface::task(dtms);
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

void ULSBusConnection::ifOk() {CN_CALL(cnclbkConnected);}

_io_op_rezult ULSBusConnection::cnSendExplorer()
{
    cnTxPacket->cmd = CN_CMD_EXPLORER;
    cnTxPacket->pld[0] = (((_cid & 0x03)<<6)| ifid());
    cnTxPacket->ridx.hope = 0;
    cnTxPacket->ridx.size = 1;
    ifTxLen = cnTxPacket->ridx.size + 1;
    return  ifSend();
}

_io_op_rezult ULSBusConnection::cnProcessExplorer()
{
    if(cnRxPacket->ridx.hope == 15 ) return IO_ERROR;// Max Hope reached
    // Forward Message
   // _connections->cnForwardExplorer(this);

    // Do answer
    cnTxPacket->cmd = CN_ACK_STATUS;
    // fix sender route
    cnTxPacket->pld[cnRxPacket->ridx.hope] = ((_cid & 0x03)<<6) | (cnRxPacket->src_did  & 0x3f);
    // Inc route size
    cnTxPacket->ridx.size = cnRxPacket->ridx.size + 1;
    // Hope = 1 for answer
    cnTxPacket->ridx.hope = 1;
    // Add current address
    cnTxPacket->pld[0] = ((_cid & 0x03)<<6) | (ifid() & 0x3f);

    // Flip route table for response
    for(int i=0;i<cnRxPacket->ridx.size;i++)
    {
       cnTxPacket->pld[i+1] =  cnRxPacket->pld[cnRxPacket->ridx.size - 1 - i];
    }

    // Add payload Information
    _cn_packet_status *px = (_cn_packet_status*)(&cnTxPacket->pld[cnTxPacket->ridx.size]);

    strcpy((char*)px->name,"TEST NAME");
    px->type = __DEVICE_TYPE;
    ifTxLen = sizeof (_cn_packet_status) + cnTxPacket->ridx.size + 1;
    return ifSend();
}
_io_op_rezult ULSBusConnection::cnForwardExplorer(ULSBusConnection *sc)
{
    if(cnRxPacket->ridx.hope == 15 ) return IO_ERROR;// Max Hope reached

    cnTxPacket->cmd = CN_CMD_EXPLORER;
    cnTxPacket->pld[sc->cnRxPacket->ridx.hope] = ((sc->cid() & 0x03)<<6) | (sc->cnRxPacket->src_did & 0x3f);
    cnTxPacket->ridx.hope = sc->cnRxPacket->ridx.hope + 1;
    cnTxPacket->ridx.size = sc->cnRxPacket->ridx.size + 1;
    ifTxLen = cnTxPacket->ridx.size + 1;
    return  ifSend();
}
_io_op_rezult ULSBusConnection::cnForwardPacket(ULSBusConnection *sc)
{
    if(cnRxPacket->ridx.hope == 15 ) return IO_ERROR;// Max Hope reached
    memcpy(cnTxPacket->pld,sc->cnRxPacket->pld,sc->ifRxLen);
    cnTxPacket->cmd = sc->cnRxPacket->cmd;
    cnTxPacket->pld[sc->cnRxPacket->ridx.hope - 1] = ((sc->cid() & 0x03)<<6) | (sc->cnRxPacket->src_did & 0x3f);
    cnTxPacket->ridx.hope = sc->cnRxPacket->ridx.hope + 1;
    ifTxLen = sc->ifRxLen + 1;
    return  ifSend();
}
_io_op_rezult ULSBusConnection::cnProcessPacket()
{
    uint8_t *R = cnRxPacket->pld;
    uint8_t  hs = cnRxPacket->ridx.size;
    uint8_t  h = cnRxPacket->ridx.hope;

    if(cnRxPacket->cmd == CN_CMD_EXPLORER)return cnProcessExplorer();
    if((R[h]&0x3f) != ifid())return IO_OK; // just not our packet - forgot about it

    if(h==(hs-1)) return cnProcessOurPacket(); // OMG it for us !!!
    _connections->cnForwardPacket(R[h]>>6,this);

    return IO_OK;
}
_io_op_rezult ULSBusConnection::cnProcessOurPacket()
{
    switch(cnRxPacket->cmd)
    {
        case CN_ACK_STATUS:
            CN_CALL(cnclbkStatusReceived);
        break;
    }


    return IO_OK;
}



_io_op_rezult ULSBusConnection::cnReceive()
{

    while(ifReceive() == IO_OK)
    {
        if(ifRxLen < 1) continue;
        ifRxLen -= 1;
        cnProcessPacket();
        ifRxLen = 0;
    }
    return IO_OK;
}
void ULSBusConnectionsList::task(uint32_t dtms)
{
    begin();
    while (next()) {
        current->task(dtms);
    }
}
void ULSBusConnectionsList::cnForwardExplorer(ULSBusConnection *sc)
{
    begin();
    while (next()) {
        if(current != sc)current->cnForwardExplorer(sc);
    }
}
_io_op_rezult ULSBusConnectionsList::cnForwardPacket(uint8_t cid,ULSBusConnection *sc)
{
    begin();
    while (next()) {
        if((current != sc)&&(current->cid() == cid)){
            return current->cnForwardPacket(sc);
        }
    }
    return IO_ERROR;
}
