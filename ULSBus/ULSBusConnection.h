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

#ifdef ULS_DBUG
#define DEBUG_MSG(MSG,...) uDebug(MSG,##__VA_ARGS__)
#define DEBUG_ERROR(FILE,LINE,MSG,...) uError(FILE,LINE,MSG,##__VA_ARGS__)
#else
#define DEBUG_MSG(MSG,...) void()
#define DEBUG_ERROR(FILE,LINE,MSG,...) (void)FILE;(void)LINE;(void)MSG;
#endif

#include "ULSBusInterface.h"

#ifndef  CN_PAYLOAD_SIZE
#define  CN_PAYLOAD_SIZE 1024
#endif



// Packet structure
typedef struct{
    uint8_t src_network;
    uint8_t src_id;
    uint8_t dsn_network;
    uint8_t dsn_id;
    uint8_t pld[CN_PAYLOAD_SIZE];
}__attribute__((packed))_cn_packet;


class ULSBusConnection:public ULSBusInterface
{
public:
      ULSBusConnection(const char* name = __null,uint8_t id = 255,uint8_t net = 255);
//    void refresh(uint8_t id);

    void deviceConnected(uint8_t id) override;
    void deviceDisconnected(uint8_t id) override;

    bool send(uint8_t cmd,uint8_t dsn_network,uint8_t dsn_id,uint32_t len);
    bool receive();

//    bool send();
//    bool sendAck(_ulsbus_ack ack,uint8_t cmd,uint8_t self_id,uint8_t remote_id);
//    bool sendNM(_ulsbus_device_status *dev);




private:
   // uint32_t _networks_timeout[255];
};


//class ULSBusConnectionsList:public ULSList<ULSBusConnection>
//{
//public:
//    ULSBusConnectionsList();

//    void redirect(ULSBusConnection* pxConnection);
//    void redirect(uint16_t dev_id,ULSBusConnection* srcConnection);
//    void sendNM(_ulsbus_device_status *dev);
//    void task();
//    void refresh(ULSBusConnection* pxConnection,uint8_t id);
//    ULSBusConnection* findId(uint8_t id);
//};


#endif // ULSBUSCONNECTION_H
