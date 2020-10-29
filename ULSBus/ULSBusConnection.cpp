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

ULSBusConnection::ULSBusConnection(IfBase* interface ,uint16_t maxFarameSize):
    ULSListItem(),
    _interface(interface),
    _maxFarameSize(maxFarameSize)
{
    for(int i = 0 ; i < 256 ; i++) _timeout[i] = 0;
};
void ULSBusConnection::refresh(uint8_t id)
{
    _timeout[id] = ULSBUS_NM_TIMEOUT;
};
void ULSBusConnection::task(){
    uint32_t *pxTimeOut = _timeout;
    uint32_t i = 255;
    while(i > 0U)
    {
        if(*pxTimeOut>0U){(*pxTimeOut)--;}
        pxTimeOut++;
        i--;
    }
    if(_interface)_interface->task();
}
uint32_t ULSBusConnection::read()
{
    return _interface->read();
};
bool ULSBusConnection::deviceConnected(uint8_t id){
    if(_timeout[id])return true;
    return false;
}
void ULSBusConnection::send(_if_buffer_instance* bufi)
{
    if(_interface)_interface->send(bufi);
}
void ULSBusConnection::send()
{
    if(_interface)_interface->send();
}
bool ULSBusConnection::sendAck(_ulsbus_ack ackcmd,uint8_t self_id,uint8_t remote_id)
{
    if(!_interface)return false;
    _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_interface->txBufInstance.buf);
    pxPack->ack.cmd = ULSBUS_ACK;
    pxPack->ack.self_id = self_id;
    pxPack->ack.remote_id = remote_id;
    pxPack->ack.ackcmd = ackcmd;
    _interface->txBufInstance.lenght = ULSBUS_HEADER_SIZE_ACK;
    _interface->send();
    return true;
}
bool ULSBusConnection::sendNM(_ulsbus_device_status *dev)
{
    if(!_interface)return false;
    _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_interface->txBufInstance.buf);
    pxPack->nm.cmd = ULSBUS_NM;
    pxPack->nm.self_id = dev->self_id;
    pxPack->nm.dev_class = dev->dev_class;
    pxPack->nm.hardware = dev->hardware;
    pxPack->nm.status1 = dev->status1;
    pxPack->nm.status2 = dev->status2;
    _interface->txBufInstance.lenght = ULSBUS_HEADER_SIZE_NM;
    _interface->send();
    return true;
}
void ULSBusConnection::interface(IfBase* interface)
{
    _interface = interface;
};
void ULSBusConnection:: maxFrameSize(uint32_t size)
{
    _maxFarameSize = size;
};
IfBase*  ULSBusConnection::interface()
{
    return _interface;
};
uint16_t ULSBusConnection::maxFrameSize()
{
    return _maxFarameSize;
};

ULSBusConnectionsList::ULSBusConnectionsList():
    ULSList()
{

};
void ULSBusConnectionsList::redirect(ULSBusConnection* pxConnection) // redirect incoming pachet from interface to other
{
    ULSBusConnection *px = head();
    while(px){
        if( px != pxConnection){
            px->send(&(pxConnection->interface()->rxBufInstance));
        }
        px = forward(px);
    };
};
void ULSBusConnectionsList::sendNM(_ulsbus_device_status *dev)
{
    ULSBusConnection *px = head();
    while(px){
        px->sendNM(dev);
        px = forward(px);
    };
};
void ULSBusConnectionsList::task()
{
    ULSBusConnection *px = head();
    while(px){
        px->task();
        px = forward(px);
    };
};
void ULSBusConnectionsList::refresh(ULSBusConnection* pxConnection,uint8_t id)
{
    ULSBusConnection *px = head();
    while(px){
        if(px == pxConnection) px->refresh(id);
        px = forward(px);
    };
};

ULSBusConnection* ULSBusConnectionsList::findId(uint8_t id)
{
    ULSBusConnection *px = head();
    while(px){
        if(px->deviceConnected(id)) return px;
        px = forward(px);
    };
    return __null;
}


