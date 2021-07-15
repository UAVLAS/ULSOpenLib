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

#ifndef ULSBUSCONNECTION_H
#define ULSBUSCONNECTION_H

#include "ULSBusInterface.h"

typedef enum {
    CN_CMD_EXPLORER = 0,
    CN_ACK_EXPLORER = 1,
    CN_CMD_SYS      = 2,
    CN_ACK_SYS      = 3,
    CN_CMD_GETOBJ   = 4,
    CN_ACK_GETOBJ   = 5,
    CN_CMD_SETOBJ   = 6,
    CN_ACK_SETOBJ   = 7
}_cn_cmd;

typedef enum :uint8_t{
    CN_SYS_CMD_PING = 0,
    CN_SYS_CMD_SETMODE = 1,
    CN_SYS_CMD_ERASE = 2,
    CN_SYS_CMD_WRITE = 3,
    CN_SYS_CMD_SETSIGNATURE = 4,
    CN_SYS_CMD_SAVECFG = 5
}_cn_sys_cmd;

typedef enum :uint8_t{
    CN_SYS_OPERATION_UNDEFINED = 0,
    CN_SYS_OPERATION_OK = 1,
    CN_SYS_OPERATION_ERROR = 2,
    CN_SYS_OPERATION_WRONGKEY = 3,

}_cn_sys_oprezult;
typedef enum :uint8_t{
    CN_SYS_MODE_APP = 0,
    CN_SYS_MODE_LOADER = 1
}_cn_sys_mode;

typedef PACKED_STRUCT(){
    uint8_t cmd;
    uint8_t src_did;
    uint8_t hop;
    uint8_t pld[256];
}_cn_packet;

typedef PACKED_STRUCT(){
    uint32_t type;
    uint8_t  name[16];
}_cn_packet_status;

typedef PACKED_STRUCT(){
    _cn_sys_cmd syscmd;
    PACKED_UNION(){
        //Sysmem messges
        PACKED_STRUCT(){
            uint16_t  devtype;
        }ping;
        PACKED_STRUCT(){
            _cn_sys_mode  mode;
        }setmode;
        PACKED_STRUCT(){
            uint32_t  key;
            uint32_t  start;
            uint32_t  len;
        }erase;
        PACKED_STRUCT(){
            uint32_t  key;
            uint32_t  start;
            uint32_t  len;
            uint8_t   buf[512];
        }rite;
        PACKED_STRUCT(){
            uint32_t  key;
            char      fw[32];
            char      ldr[32];
            uint32_t  progflashingtime;
            uint32_t  progsize;
            uint32_t  progcrc;
        }ignature;
        PACKED_STRUCT(){
            uint32_t  key;
        }saveCfg;

    };
}cn_sys_packet;


class ULSBusConnection;
typedef void (*_uls_cn_callback)(ULSBusConnection*);
typedef _cn_sys_oprezult (*_uls_cn_sys_callback)(ULSBusConnection*,_cn_sys_packet*);
typedef void (*_uls_cn_sysack_callback)(ULSBusConnection*,_cn_sys_oprezult);

#define CN_CALL(func) if(func !=nullptr)func(this);
#define CN_CALL_SYS(func) (func !=nullptr)?func(this,(_cn_sys_packet*)(&cnRxPacket->pld[cnRxPacket->hop & 0xF])):CN_SYS_OPERATION_ERROR;
#define CN_CALL_SYSACK(func) if(func !=nullptr)func(this,(_cn_sys_oprezult)cnRxPacket->pld[cnRxPacket->hop & 0xF]);

class ULSBusConnectionsList:public ULSList<ULSBusConnection>
{
public:
    ULSBusConnectionsList(){};
    void cnForwardExplorer(ULSBusConnection *sc);
    _io_op_rezult cnForwardPacket(uint8_t cid,ULSBusConnection *sc);
    _io_op_rezult cnSendGetObject(uint8_t *route,uint8_t hs,uint16_t obj_addr);
    _io_op_rezult cnSendSetObject(uint8_t *route, uint8_t hs, uint16_t obj_addr, uint8_t *buf, uint32_t size);
    _io_op_rezult cnSendSysSetMode(uint8_t *route,uint8_t hs,_cn_sys_mode mode);
    _io_op_rezult cnSendSysErase(uint8_t *route,uint8_t hs,uint32_t key,uint32_t start, uint32_t len);
    _io_op_rezult cnSendSysWrite(uint8_t *route,uint8_t hs,uint32_t key,uint32_t start, uint32_t len,uint8_t *buf);
    _io_op_rezult cnSendSysSaveConfig(uint8_t *route,uint8_t hs,uint32_t key);
    _io_op_rezult cnSendSysSetSignature(uint8_t *route,uint8_t hs,uint32_t key,char* fw,
                                        char* ldr,uint32_t ftime,uint32_t progsize,uint32_t progcrc);

    _io_op_rezult cnSetClbkSys(_uls_cn_sys_callback func);
    _io_op_rezult cnSetClbkSysAck(_uls_cn_sysack_callback func);
    _io_op_rezult cnSendExplorer();
    _io_op_rezult open();
    void close();

    _io_op_rezult setDID(uint8_t cid, uint8_t did);

    void task(uint32_t dtms);
};

class ULSBusConnection: public ULSListItem, public ULSBusInterface
{
public:
    ULSBusConnection(ULSDBase *dev,ULSBusConnectionsList* connections,const char* name = __null,uint8_t did = 255,uint8_t cid = 0);

    _cn_packet *cnRxPacket;
    _cn_packet *cnTxPacket;

    virtual void task(uint32_t dtms);
    void deviceConnected(uint8_t id) override;
    void deviceDisconnected(uint8_t id) override;
    void ifOk() override;

    _io_op_rezult cnSendExplorer();
    _io_op_rezult cnSendStatus();
    _io_op_rezult cnSendGetObject(uint8_t *route,uint8_t hs,uint16_t obj_addr);
    _io_op_rezult cnSendSetObject(uint8_t *route, uint8_t hs, uint16_t obj_addr, uint8_t *buf, uint32_t size);
    _io_op_rezult cnSendSysSetMode(uint8_t *route,uint8_t hs,_cn_sys_mode mode);
    _io_op_rezult cnSendSysErase(uint8_t *route,uint8_t hs,uint32_t key,uint32_t start, uint32_t len);
    _io_op_rezult cnSendSysWrite(uint8_t *route,uint8_t hs,uint32_t key,uint32_t start, uint32_t len,uint8_t *buf);
    _io_op_rezult cnSendSysSaveConfig(uint8_t *route,uint8_t hs,uint32_t key);
    _io_op_rezult cnSendSysSetSignature(uint8_t *route,uint8_t hs,uint32_t key,char* fw,char* ldr,uint32_t ftime,uint32_t progsize,uint32_t progcrc);

    _io_op_rezult cnProcessPacket();
    _io_op_rezult cnProcessOurPacket();
    _io_op_rezult cnProcessExplorer();
    _io_op_rezult cnProcessSys();
    _io_op_rezult cnProcessGetObject();
    _io_op_rezult cnProcessSetObject();
    _io_op_rezult cnProcessStatus();

    _io_op_rezult cnForwardExplorer(ULSBusConnection *src);
    _io_op_rezult cnForwardPacket(ULSBusConnection *src);


    bool send(uint8_t cmd,uint8_t dsn_network,uint8_t dsn_id,uint32_t len);
    _io_op_rezult cnReceive();


    uint8_t cnrid(){return ((_cid & 0x03)<<6)| ifid();}
    uint8_t cnrid(uint8_t cid){return (((cid&0x03)<<6) | ifid());};
    uint8_t cid(){return _cid;}

    _uls_cn_callback cnclbkConnected;
    _uls_cn_callback cnclbkStatusReceived;
    _uls_cn_callback cnclbkObjReceived;
    _uls_cn_callback cnclbkObjSended;
    _uls_cn_callback cnclbkObjRequested;
    _uls_cn_sys_callback cnclbkSys;
    _uls_cn_sysack_callback cnclbkSysAck;

private:
    uint8_t *cnPrepareAnswer(uint8_t cmd);
    uint8_t *cnPreparePacket(uint8_t *route,uint8_t hs,uint8_t cmd);
private:

    uint8_t     _cid;
    ULSBusConnectionsList* _connections;

};

#endif // ULSBUSCONNECTION_H
