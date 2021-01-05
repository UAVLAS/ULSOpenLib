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
 * Some open source application is distributed in the hop that it will
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

ULSBusConnection::ULSBusConnection(ULSDBase *dev,ULSBusConnectionsList* connections,const char* name,uint8_t did,uint8_t cid):
    ULSListItem(),
    ULSBusInterface(name,did),
    cnclbkConnected(nullptr),
    cnclbkStatusReceived(nullptr),
    cnclbkObjReceived(nullptr),
    cnclbkObjSended(nullptr),
    cnclbkObjRequested(nullptr),
    _cid(cid),
    _connections(connections)
{
    _dev = dev;
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
    DEBUG_MSG("%s: Device Connected 0x%.2X ",_name,id);
    (void)id;
}
void ULSBusConnection::deviceDisconnected(uint8_t id)
{
    DEBUG_MSG("%s: Device Disconnected 0x%.2X ",_name,id);
    (void)id;
}

void ULSBusConnection::ifOk() {CN_CALL(cnclbkConnected);}

_io_op_rezult ULSBusConnection::cnReceive()
{
    while(ifReceive() == IO_OK){
        if(ifRxLen < 1) continue;
        //  DEBUG_MSG("%s: cnReceived from:0x%.2X cmd: 0x%.2X len: %d",_name,cnRxPacket->src_did ,cnRxPacket->cmd,ifRxLen);
        cnProcessPacket();
        ifRxLen = 0;
    }
    return IO_OK;
}
_io_op_rezult ULSBusConnection::cnProcessPacket()
{
    uint8_t *R = cnRxPacket->pld;
    uint32_t rxH = cnRxPacket->hop >> 4;
    uint32_t rxHs = cnRxPacket->hop & 0xF;
    uint8_t  rId = R[rxH]&0x3f;
    uint8_t  forwardCID = R[rxH+1]>>6;

    // Fix Route with  source information
    R[rxH] =  (((_cid & 0x03)<<6)| (cnRxPacket->src_did&0x3f));
    if(cnRxPacket->cmd == CN_CMD_EXPLORER)return cnProcessExplorer();

    //    DEBUG_MSG("%s: cnProcessPacket lid:0x%.2X cmd: 0x%.2X len: %d hs:%d h:%d",_name,_did,cnTxPacket->cmd,ifTxLen,rxHs,rxH);
    //    DEBUG_PACKET(_name," cnProcessPacket Route",cnRxPacket->pld,rxHs);
    //    DEBUG_MSG("%s: cnProcessPacket Check route id [%.2X] vs Self [%.2X]",_name,rId,ifid());

    if(rId != ifid())return IO_OK; // just not our packet - forgot about it
    if((rxH + 1) == rxHs) return cnProcessOurPacket(); // OMG it for us !!!
    _connections->cnForwardPacket(forwardCID,this);
    return IO_OK;
}
uint8_t *ULSBusConnection::cnPrepareAnswer(uint8_t cmd)
{
    uint32_t rxHs = cnRxPacket->hop & 0xF;
    cnTxPacket->cmd = cmd;
    for(uint32_t i=0 ; i < rxHs ; i++){    // Flip route table for response
        cnTxPacket->pld[i] =  cnRxPacket->pld[rxHs - 1 - i];
    }
    cnTxPacket->hop = (0 << 4) | rxHs;    // hop  =  0; hop size = rxhop
    return &cnTxPacket->pld[cnTxPacket->hop & 0xF];
}
_io_op_rezult ULSBusConnection::cnProcessExplorer()
{
    if((cnRxPacket->hop & 0xF) < 15 ){ // Max hop reached no forwarding
        _connections->cnForwardExplorer(this);    // Forward Message
    }
    // Prepare answer
    _cn_packet_status *px = (_cn_packet_status*)cnPrepareAnswer(CN_ACK_EXPLORER);

    memcpy((uint8_t*)px->name,_dev->devname,16);
    px->name[15] = 0;
    px->type = _dev->typeCode;
    uint32_t txHs = cnTxPacket->hop&0xF;
    ifTxLen = sizeof (_cn_packet_status) + txHs + 1;
    //    DEBUG_MSG("%s: cnAnswer lid:0x%.2X cmd: 0x%.2X len: %d",_name,_did,cnTxPacket->cmd,ifTxLen);
    //    DEBUG_PACKET(_name,"cnAnswer Route",cnTxPacket->pld,txHs);
    return ifSend();
}
_io_op_rezult ULSBusConnection::cnProcessGetObject()
{
    uint16_t obj_id =  *((uint16_t*)&cnRxPacket->pld[cnRxPacket->hop&0xf]);

    ULSObjectBase* obj = _dev->getObject(obj_id);
    if(obj == nullptr) {
        DEBUG_MSG("%s: Requested Wrong Object [0x%.4X]",_name,obj_id);
        return IO_ERROR;
    }
    if((obj->_permition != ULSBUS_OBJECT_PERMITION_READWRITE)&&
            (obj->_permition != ULSBUS_OBJECT_PERMITION_READONLY)) return IO_ERROR;

    uint8_t *px = cnPrepareAnswer(CN_ACK_GETOBJ);
    *((uint16_t*)px) = obj_id;
    px+=2;
    obj->getData(px);
    uint32_t txHs = cnTxPacket->hop&0xF;
    ifTxLen = 1 + txHs + 2 + obj->size;
    //  DEBUG_MSG("%s: cnAnswer Object lid:0x%.2X cmd: 0x%.2X len: %d",_name,_did,cnTxPacket->cmd,ifTxLen);
    //  DEBUG_PACKET(_name,"cnAnswer Route",cnTxPacket->pld,txHs);
    return ifSend();

}
_io_op_rezult ULSBusConnection::cnProcessSetObject()
{
    uint16_t obj_id =  *((uint16_t*)&cnRxPacket->pld[cnRxPacket->hop&0xf]);
    uint8_t *obj_px = ((uint8_t*)&cnRxPacket->pld[(cnRxPacket->hop&0xf) + 2]);

    ULSObjectBase* obj = _dev->getObject(obj_id);
    if(obj == nullptr) {
        DEBUG_MSG("%s: Requested Wrong Object [0x%.4X]",_name,obj_id);
        return IO_ERROR;
    }
    if((obj->_permition != ULSBUS_OBJECT_PERMITION_READWRITE)&&
            (obj->_permition != ULSBUS_OBJECT_PERMITION_WRITEONLY))return IO_ERROR;

    obj->setData(obj_px);
    uint8_t *px = cnPrepareAnswer(CN_ACK_SETOBJ);
    *((uint16_t*)px) = obj_id;

    uint32_t txHs = cnTxPacket->hop&0xF;
    ifTxLen = 1 + txHs + 2;
    return ifSend();
}
_io_op_rezult ULSBusConnection::cnProcessSys()
{
    uint8_t *px = cnPrepareAnswer(CN_ACK_SYS);

    volatile _cn_sys_oprezult rez = CN_CALL_SYS(cnclbkSys);
    *((_cn_sys_oprezult*)px) = rez;

    uint32_t txHs = cnTxPacket->hop&0xF;
    ifTxLen = 1 + txHs + 1;
    return ifSend();
}
_io_op_rezult ULSBusConnection::cnProcessOurPacket()
{
    switch(cnRxPacket->cmd)
    {
    case CN_ACK_EXPLORER:
        CN_CALL(cnclbkStatusReceived);
        break;
    case CN_CMD_SYS:
        cnProcessSys();
        break;
    case CN_ACK_SYS:
        CN_CALL_SYSACK(cnclbkSysAck);
        break;
    case CN_CMD_GETOBJ:
        cnProcessGetObject();
        break;
    case CN_ACK_GETOBJ:
        CN_CALL(cnclbkObjReceived);
        break;
    case CN_CMD_SETOBJ:
        cnProcessSetObject();
        break;
    case CN_ACK_SETOBJ:
        CN_CALL(cnclbkObjSended);
        break;
    }
    return IO_OK;
}
uint8_t *ULSBusConnection::cnPreparePacket(uint8_t *route,uint8_t hs,uint8_t cmd)
{
    cnTxPacket->cmd = cmd;
    cnTxPacket->hop = (0<<4) | hs;
    memcpy(cnTxPacket->pld,route,hs);
    ifTxLen = 1 + hs;
    return &cnTxPacket->pld[hs];
}
_io_op_rezult ULSBusConnection::cnSendExplorer()
{
    cnTxPacket->cmd = CN_CMD_EXPLORER;
    cnTxPacket->hop = (0<<4) | 1; // h = 0; hs = 1;
    ifTxLen = (cnTxPacket->hop&0xF) + 1;
    return  ifSend();
}
 _io_op_rezult ULSBusConnection::cnSendSysErase(uint8_t *route,uint8_t hs,uint32_t key,uint32_t start, uint32_t len)
{
     if( (route[0] >> 6) != _cid)return IO_ERROR; // not our interface
     _cn_sys_packet *pxsys = (_cn_sys_packet*)cnPreparePacket(route,hs,CN_CMD_SYS);
     pxsys->syscmd = CN_SYS_CMD_ERASE;
     pxsys->erase.key = key;
     pxsys->erase.start = start;
     pxsys->erase.len = len;
     ifTxLen += + 1 + 12;
     return  ifSend();
}
 _io_op_rezult ULSBusConnection::cnSendSysSetMode(uint8_t *route,uint8_t hs,_cn_sys_mode mode)
{
     if( (route[0] >> 6) != _cid)return IO_ERROR; // not our interface
     _cn_sys_packet *pxsys = (_cn_sys_packet*)cnPreparePacket(route,hs,CN_CMD_SYS);
     pxsys->syscmd = CN_SYS_CMD_SETMODE;
     pxsys->setmode.mode = mode;
     ifTxLen += 1 + 1;
     return  ifSend();
}
 _io_op_rezult ULSBusConnection::cnSendSysWrite(uint8_t *route,uint8_t hs,uint32_t key,uint32_t start, uint32_t len,uint8_t *buf)
{
     if( (route[0] >> 6) != _cid)return IO_ERROR; // not our interface
     if(len > 512) return IO_ERROR; //buff too big
     _cn_sys_packet *pxsys = (_cn_sys_packet*)cnPreparePacket(route,hs,CN_CMD_SYS);
     pxsys->syscmd = CN_SYS_CMD_WRITE;
     pxsys->write.key = key;
     pxsys->write.start = start;
     pxsys->write.len = len;
     memcpy(pxsys->write.buf,buf,len);
     ifTxLen += 1 + 12 + len;
     return  ifSend();
}
 _io_op_rezult ULSBusConnection::cnSendSysSetSignature(uint8_t *route,uint8_t hs,uint32_t key,char* fw,
                                                       char* ldr,uint32_t ftime,uint32_t progsize,uint32_t progcrc)
{
     if( (route[0] >> 6) != _cid)return IO_ERROR; // not our interface
     _cn_sys_packet *pxsys = (_cn_sys_packet*)cnPreparePacket(route,hs,CN_CMD_SYS);
     pxsys->syscmd = CN_SYS_CMD_SETSIGNATURE;
     pxsys->signature.key = key;
     memcpy(pxsys->signature.fw,fw,16);
     memcpy(pxsys->signature.ldr,ldr,16);
     pxsys->signature.progflashingtime = ftime;
     pxsys->signature.progsize = progsize;
     pxsys->signature.progcrc = progcrc;
     ifTxLen += 1 + 12 + 16 + 64;
     return  ifSend();
}
_io_op_rezult ULSBusConnection::cnSendGetObject(uint8_t *route,uint8_t hs,uint16_t obj_addr)
{
    if( (route[0] >> 6) != _cid)return IO_ERROR; // not our interface

    cnTxPacket->cmd = CN_CMD_GETOBJ;
    cnTxPacket->hop = (0<<4) | hs;
    memcpy(cnTxPacket->pld,route,hs);

    cnTxPacket->pld[hs]   = obj_addr&0xff;
    cnTxPacket->pld[hs+1] = (obj_addr>>8)&0xff;
    ifTxLen = (1 + hs + 2);
    return  ifSend();
}
_io_op_rezult ULSBusConnection::cnSendSetObject(uint8_t *route,uint8_t hs,uint16_t obj_addr,uint8_t *buf,uint32_t size)
{
    if( (route[0] >> 6) != _cid)return IO_ERROR; // not our interface

    cnTxPacket->cmd = CN_CMD_SETOBJ;
    cnTxPacket->hop = (0<<4) | hs;
    memcpy(cnTxPacket->pld,route,hs);

    cnTxPacket->pld[hs]   = obj_addr&0xff;
    cnTxPacket->pld[hs+1] = (obj_addr>>8)&0xff;
    memcpy(&cnTxPacket->pld[hs+2],buf,size);
    ifTxLen = (1 + hs + 2 + size);
    return  ifSend();
}
_io_op_rezult ULSBusConnection::cnForwardExplorer(ULSBusConnection *sc)
{
    uint32_t rxHs = sc->cnRxPacket->hop & 0xF;

    if(rxHs == 15 ) return IO_ERROR;// Max hop reached

    cnTxPacket->cmd = CN_CMD_EXPLORER;
    // Copy roure
    for(uint32_t i = 0 ; i < rxHs ; i++){
        cnTxPacket->pld[i] =  sc->cnRxPacket->pld[i];
    }
    cnTxPacket->hop = sc->cnRxPacket->hop + 0x11; // Inc hop and hopSize
    // DEBUG_MSG("%s: cnForward Explorer Added route: %.2X",_name,cnTxPacket->pld[rxHs]);
    ifTxLen = (cnTxPacket->hop & 0xF) + 1;
    //  DEBUG_MSG("%s: cnForward Explorer lid:0x%.2X cmd: 0x%.2X len: %d",_name,_did,cnTxPacket->cmd,ifTxLen);
    return  ifSend();
}
_io_op_rezult ULSBusConnection::cnForwardPacket(ULSBusConnection *sc)
{
    memcpy(cnTxPacket->pld,sc->cnRxPacket->pld,sc->ifRxLen-1);
    cnTxPacket->cmd = sc->cnRxPacket->cmd;
    cnTxPacket->hop = sc->cnRxPacket->hop + 0x10;

    ifTxLen = sc->ifRxLen;
    // DEBUG_MSG("%s: cnForward lid:0x%.2X cmd: 0x%.2X len: %d",_name,_did,cnTxPacket->cmd,ifTxLen);
    return  ifSend();
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


_io_op_rezult ULSBusConnectionsList::cnSendGetObject(uint8_t *route,uint8_t hs,uint16_t obj_addr)
{
    begin();
    while (next()) {
        if (current->cnSendGetObject(route,hs,obj_addr) == IO_OK) return IO_OK;
    }
    return IO_ERROR;
}
_io_op_rezult ULSBusConnectionsList::cnSendSetObject(uint8_t *route,uint8_t hs,uint16_t obj_addr,uint8_t *buf,uint32_t size)
{
    begin();
    while (next()) {
        if (current->cnSendSetObject(route,hs,obj_addr,buf,size) == IO_OK) return IO_OK;
    }
    return IO_ERROR;
}
_io_op_rezult ULSBusConnectionsList::cnSendExplorer()
{
    begin();
    while (next()) {
        if (current->cnSendExplorer() != IO_OK) return IO_ERROR;
    }
    return IO_OK;
}
_io_op_rezult ULSBusConnectionsList::open()
{
    begin();
    while (next()) {
        current->open();
    }
    return IO_OK;
}
_io_op_rezult ULSBusConnectionsList::setDID(uint8_t cid, uint8_t did)
{
    begin();
    while (next()) {
        if(current->cid() == cid)current->ifid(did);
    }
    return IO_OK;
}

_io_op_rezult ULSBusConnectionsList::cnSendSysSetMode(uint8_t *route,uint8_t hs,_cn_sys_mode mode)
{
    begin();
    while (next()) {
        if(current->cnSendSysSetMode(route,hs,mode) == IO_OK) return IO_OK;
    }
    return IO_ERROR;
}
_io_op_rezult ULSBusConnectionsList::cnSendSysErase(uint8_t *route,uint8_t hs,uint32_t key,uint32_t start, uint32_t len)
{
    begin();
    while (next()) {
        if(current->cnSendSysErase(route,hs,key,start,len) == IO_OK) return IO_OK;
    }
    return IO_ERROR;
}
_io_op_rezult ULSBusConnectionsList::cnSendSysWrite(uint8_t *route,uint8_t hs,uint32_t key,uint32_t start, uint32_t len,uint8_t *buf)
{
    begin();
    while (next()) {
        if(current->cnSendSysWrite(route,hs,key,start,len,buf) == IO_OK) return IO_OK;
    }
    return IO_ERROR;
}
_io_op_rezult ULSBusConnectionsList::cnSendSysSetSignature(uint8_t *route,uint8_t hs,uint32_t key,char* fw,
                                                           char* ldr,uint32_t ftime,uint32_t progsize,uint32_t progcrc)
{
    begin();
    while (next()) {
        if(current->cnSendSysSetSignature(route,hs,key,fw,ldr,ftime,progsize,progcrc) == IO_OK) return IO_OK;
    }
    return IO_ERROR;
}
_io_op_rezult ULSBusConnectionsList::cnSetClbkSys(_uls_cn_sys_callback func)
{
    begin();
    while (next()) {
        current->cnclbkSys = func;
    }
    return IO_OK;
}
_io_op_rezult ULSBusConnectionsList::cnSetClbkSysAck(_uls_cn_sysack_callback func)
{
    begin();
    while (next()) {
        current->cnclbkSysAck = func;
    }
    return IO_OK;
}
