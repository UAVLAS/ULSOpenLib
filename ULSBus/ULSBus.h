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

#ifndef ULSBUS_H
#define ULSBUS_H

#include <string.h>
#include "ULSBusTypes.h"
#include "ULSBusTransaction.h"
#include "ULSBusConnection.h"
#include "ULSBusObject.h"

class ULSBus
{
public:
    ULSBus(ULSBusObjectsLibrary *library);
    void task();
    void open();
    void sendNM();
    bool processAck(ULSBusConnection* pxConnection);
    bool processPacket(ULSBusConnection *pxConnection);
    uint32_t openedTransactions();
    void add(ULSBusTransaction* transaction);
    void add(ULSBusConnection* connection);
    void add(ULSBusObjectBuffer* buf);
    void add(ULSBusTransaction* transaction, uint32_t len);
    void add(ULSBusConnection* connection, uint32_t len);
    void add(ULSBusObjectBuffer* buf, uint32_t len);

private:
    ULSBusTransactionsList _tarnsactions;
    ULSBusConnectionsList _connections;
    ULSBusObjectsLibrary    *_library;
    ULSBusObjectBufferList  _oBuf;

    uint32_t _nmTimeout;
};

#endif // ULSBUS_H
