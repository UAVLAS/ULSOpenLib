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

#include "ULSBusTypes.h"

class ULSBusConnection:public ULSListItem
{
public:
    ULSBusConnection(IfBase* interface = __null);
    void refresh(uint8_t id);
    void task();
    uint32_t read();
    bool deviceConnected(uint8_t id);
    bool send(_if_buffer_instance* bufi);
    bool send();
    bool sendAck(_ulsbus_ack ack,uint8_t cmd,uint8_t self_id,uint8_t remote_id);
    bool sendNM(_ulsbus_device_status *dev);
    void interface(IfBase* interface);
    IfBase* interface();
    uint32_t maxFrameSize();

private:
    IfBase* _interface;
    uint32_t _timeout[256];
};


class ULSBusConnectionsList:public ULSList<ULSBusConnection>
{
public:
    ULSBusConnectionsList();

    void redirect(ULSBusConnection* pxConnection);
    void redirect(uint16_t dev_id,ULSBusConnection* srcConnection);
    void sendNM(_ulsbus_device_status *dev);
    void task();
    void refresh(ULSBusConnection* pxConnection,uint8_t id);
    ULSBusConnection* findId(uint8_t id);
};


#endif // ULSBUSCONNECTION_H
