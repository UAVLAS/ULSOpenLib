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
#include "ULSDevice_ULSQX.h"
//#include "ULSObject.h"


class ULSBusConnection;
typedef void (*_uls_cn_callback)(ULSBusConnection*);

#define CN_CALL(func) if(func !=nullptr)func(this);


// Packet structure
typedef struct{
    uint8_t cmd;
    uint8_t src_did;
    uint8_t hop;
    uint8_t pld[256];
}__attribute__((packed))_cn_packet;


typedef struct{
    uint32_t type;
    uint8_t  name[16];
}__attribute__((packed))_cn_packet_status;


//typedef struct
//{
//    uint32_t    lenght;
//    _cn_packet  *packet;
//}_cn_instance;


typedef enum {
    CN_CMD_EXPLORER = 0,
    CN_ACK_EXPLORER = 1,
    CN_CMD_GETOBJ   = 2,
    CN_ACK_GETOBJ   = 3,
    CN_CMD_SETOBJ   = 4,
    CN_ACK_SETOBJ   = 5,
}_cn_cmd;



class ULSBusConnectionsList:public ULSList<ULSBusConnection>
{
public:
    ULSBusConnectionsList(){};
    void cnForwardExplorer(ULSBusConnection *sc);
    _io_op_rezult cnForwardPacket(uint8_t cid,ULSBusConnection *sc);
    _io_op_rezult cnSendGetObject(uint8_t *route,uint8_t hs,uint16_t obj_addr);
    _io_op_rezult cnSendSetObject(uint8_t *route,uint8_t hs,uint16_t obj_addr,uint8_t *buf);
    _io_op_rezult cnSendExplorer();
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
    _io_op_rezult cnSendGetObject(uint8_t *route,uint8_t hs,uint16_t obj_addr);
    _io_op_rezult cnSendSetObject(uint8_t *route,uint8_t hs,uint16_t obj_addr,uint8_t *buf);

    _io_op_rezult cnProcessExplorer();
    _io_op_rezult cnForwardExplorer(ULSBusConnection *src);

    _io_op_rezult cnProcessPacket();
    _io_op_rezult cnProcessOurPacket();
    _io_op_rezult cnForwardPacket(ULSBusConnection *src);
    _io_op_rezult cnProcessGetObject();
    _io_op_rezult cnProcessSetObject();

    _io_op_rezult cnSendStatus();
    _io_op_rezult cnProcessStatus();




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



protected:
    //    virtual _if_op_rezult sendPacket(){return IF_ERROR;};
    //    virtual _if_op_rezult receivePacket(){return IF_ERROR;};
private:
    uint8_t *cnPrepareAnswer(uint8_t cmd);
private:

    uint8_t     _cid;
    ULSBusConnectionsList* _connections;
    ULSDBase *_dev;


    // uint32_t _networks_timeout[255];
};




#endif // ULSBUSCONNECTION_H
