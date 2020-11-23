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

#ifndef ULSBUSTRANSACTION_H
#define ULSBUSTRANSACTION_H
#include "IfBase.h"
#include "ULSBusConnection.h"
#include "ULSBusObjectBuffer.h"
#include "ULSDevicesLibrary.h"


class ULSBusTransaction:public ULSListItem{
public:
    ULSBusTransaction();
    void library(ULSDevicesLibrary    *library);

    bool open(ULSBusConnection* connection,uint8_t selfId,uint8_t remoteId);
    bool open(ULSBusConnection* connection,ULSBusConnection* connectionGate,uint8_t selfId,uint8_t remoteId);
    void close();
    void close(uint8_t cmd,uint8_t remoteID);
    void connectBuffer(ULSBusObjectBuffer* buf);
    void disconnectBuffer();
    bool check(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id,_ulsbus_transaction_state state);

    bool processPacket();
    bool task() ;

    void state(_ulsbus_transaction_state state);
    _ulsbus_transaction_state state();
    ULSBusObjectBuffer* buffer();

private:
    bool boiTransmitStart();
    bool rwoiTransmitStart();
    bool aoiTransmitStart();
    void initFrames();
private:
    ULSBusObjectBuffer* _buf;     // Pointer to object buffers
    ULSBusConnection *_connection; // Pointer to main Connection
    ULSBusConnection *_connectionGate; // Pointer to gate connection Connection

    ULSDevicesLibrary    *_library;
    _ulsbus_transaction_state _state;
    uint32_t _timeout;
    uint8_t  _cmd;
    uint8_t  _self_id;
    uint8_t  _remote_id;
    uint32_t  _frames;
    uint32_t  _frameNum;
    uint32_t  _frameSize;
    uint32_t  _frameLastSize;
    //    uint16_t _crc;
};

class ULSBusTransactionsList:public ULSList<ULSBusTransaction>{
public:
    ULSBusTransactionsList();

    void library(ULSDevicesLibrary    *library);
    void task();
    ULSBusTransaction* open(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id);

    ULSBusTransaction* open(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id,
                            ULSBusObjectBuffer* buf,_ulsbus_transaction_state state);

    ULSBusTransaction* open(ULSBusConnection* connection,ULSBusConnection* connectionGate,
                            uint8_t self_id,uint8_t remote_id,ULSBusObjectBuffer* buf,
                            _ulsbus_transaction_state state);
    ULSBusTransaction* find(ULSBusConnection* connection,uint8_t self_id,uint8_t remote_id,
                            _ulsbus_transaction_state state);
    uint32_t openedTransactions();

};
#endif // ULSBUSTRANSACTION_H
