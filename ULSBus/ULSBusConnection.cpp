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

ULSBusConnection::ULSBusConnection(const char* name,uint8_t id,uint8_t net):
    //ULSListItem(),
    ULSBusInterface(name,id,net)
{
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
bool ULSBusConnection::send(uint8_t cmd,uint8_t dsn_network,uint8_t dsn_id,uint32_t len)
{

    _cn_packet *txPacket = (_cn_packet *)(txInstance.packet.pld);

    txPacket->dsn_id = dsn_id;
    txPacket->dsn_network = dsn_network;
    txPacket->src_id = _id;
    txPacket->src_network = _network;

    if(sendPacket(cmd|0x10,len + 4)== IF_OK)return true;

    return false;
}
bool ULSBusConnection::receive()
{
    if(ULSBusInterface::receivePacket() == IF_OK) return true;
    return false;
}
