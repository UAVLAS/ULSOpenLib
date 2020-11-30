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

ULSBusConnection::ULSBusConnection(IfBase* interface):
    ULSListItem(),
    _interface(interface)
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
bool ULSBusConnection::send(_if_buffer_instance* bufi)
{
    if(_interface)return _interface->send(bufi);
        ULSBUS_ERROR("Interface %s not Valid",_interface->name());
    return false;
}
bool ULSBusConnection::send()
{
    if(_interface == __null){
        ULSBUS_ERROR("Interface %s not Valid",_interface->name());
        return false;
      }
      bool rez = _interface->send();

   if(rez == false)
   {
      ULSBUS_ERROR("Interface %s send() failure",_interface->name());
   }
   return rez;
}
bool ULSBusConnection::sendAck(_ulsbus_ack ack,uint8_t cmd,uint8_t self_id,uint8_t remote_id)
{
    if(!_interface)return false;
    _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_interface->txBufInstance.buf);
    pxPack->ack.cmd = ULSBUS_ACK;
    pxPack->ack.self_id = self_id;
    pxPack->ack.remote_id = remote_id;
    pxPack->ack.ackcmd = (ack<<5)|cmd;
    _interface->txBufInstance.lenght = ULSBUS_HEADER_SIZE_ACK;
    return  _interface->send();
}
bool ULSBusConnection::sendNM(_ulsbus_device_status *dev)
{
    if(!_interface)return false;
    _ulsbus_packet  *pxPack = (_ulsbus_packet *)(_interface->txBufInstance.buf);
    pxPack->nm.cmd = ULSBUS_NM;
    pxPack->nm.self_id = dev->id;
    pxPack->nm.dev_class = dev->devClass;
    pxPack->nm.hardware = dev->hardware;
    pxPack->nm.status1 = dev->status1;
    pxPack->nm.status2 = dev->status2;
    _interface->txBufInstance.lenght = ULSBUS_HEADER_SIZE_NM;
    return _interface->send();
}
void ULSBusConnection::interface(IfBase* interface)
{
    _interface = interface;
};
IfBase*  ULSBusConnection::interface()
{
    return _interface;
};
uint32_t ULSBusConnection::maxFrameSize()
{
    return _interface->maxFrameSize();
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
void ULSBusConnectionsList::redirect(uint16_t dev_id,ULSBusConnection* srcConnection) // redirect incoming pachet to specifed ID device
{
    ULSBusConnection* pxc =  findId(dev_id);
    if(!pxc) return; // connection with device not found
    // Redirect request to next device;
    if( pxc != srcConnection){
        pxc->send(&(srcConnection->interface()->rxBufInstance));
    }
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


