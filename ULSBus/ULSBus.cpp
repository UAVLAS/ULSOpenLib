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

#include "ULSBus.h"

ULSBus::ULSBus(const char* name,ULSDeviceBase *selfDevice):
    _selfDevice(selfDevice),
    _name(name),
    _time(0),
    _nmTimeout(0)
{
    if(_selfDevice)_selfDevice->connected(true);
};
void ULSBus::task()// call every ms
{
    _nmTimeout++;
    _time++;
    _connections.task();
    _tarnsactions.task();
    ULSBusConnection *pxConnection = _connections.head();
    while(pxConnection){
        while( pxConnection->read() != 0){ // read all packets from connection
            processPacket(pxConnection);
        }
        pxConnection = _connections.forward(pxConnection);
    }
    // Send NM packet
    if(_nmTimeout >= ULSBUS_NM_INTERVAL){
        sendNM();
        _nmTimeout = 0;
    }
}
void ULSBus::open()
{

}
bool ULSBus::sendObject(ULSBusObjectBase* obj)
{
    if(obj->permition() == ULSBUS_OBJECT_PERMITION_WRITEONLY){ // why to read  write only object WTF ?
        obj->state(ULSBUS_OBJECT_STATE_ERROR);
        return false;
    }
    ULSDeviceBase* remoteDevice = (ULSDeviceBase*)obj->parent();
    // check if we have devices connected to us with corresponding ID
    ULSBusConnection* pxc =  _connections.findId(remoteDevice->id());
    if(!pxc)
    {
        obj->state(ULSBUS_OBJECT_STATE_ERROR);
        return false; //  connection not found
    }
    // Create Transaction Buffer
    ULSBusObjectBuffer* buffer =_oBuf.open(obj->id(),obj->size(),obj);
    if(!buffer)
    {
        obj->state(ULSBUS_OBJECT_STATE_ERROR);
        return false;
    }
    buffer->setData(obj->data(),obj->size());
    ULSBusTransaction* pxt = _tarnsactions.open(pxc,_selfDevice->id(),remoteDevice->id(),buffer,ULSBUST_RWOI_TRANSMIT_START);
    if(!pxt){
        obj->state(ULSBUS_OBJECT_STATE_ERROR);
        return false;
    }
    ULSBUS_LOG("SEND Object   : self_id: 0x%X remote_id: 0x%X object: 0x%X",_selfDevice->id(),remoteDevice->id(),obj->id());
    obj->state(ULSBUS_OBJECT_STATE_BUSY);
    return true;
}
bool ULSBus::requestObject(ULSBusObjectBase* obj)
{
    if(obj->permition() == ULSBUS_OBJECT_PERMITION_READONLY){ // why to write readonly object WTF ?
        obj->state(ULSBUS_OBJECT_STATE_ERROR);
        return false;
    }
    ULSDeviceBase* remoteDevice = (ULSDeviceBase*)obj->parent();
    // check if we have devices connected to us with corresponding ID
    ULSBusConnection* pxc =  _connections.findId(remoteDevice->id());
    if(!pxc)return false; //  connection not found
    _ulsbus_packet  *pxTxPack = (_ulsbus_packet *)(pxc->interface()->txBufInstance.buf);

    pxTxPack->rroi.cmd = ULSBUS_RROI;
    pxTxPack->rroi.self_id = _selfDevice->id();
    pxTxPack->rroi.remote_id = remoteDevice->id();
    pxTxPack->rroi.obj_id = obj->id();// PC adress
    pxTxPack->rroi.size = obj->size();
    pxc->interface()->txBufInstance.lenght = ULSBUS_HEADER_SIZE_RROI;
    ULSBUS_LOG("Request Object: self_id: 0x%X remote_id: 0x%X object: 0x%X",_selfDevice->id(),remoteDevice->id(),obj->id());
    if(!pxc->send())
    {
        obj->state(ULSBUS_OBJECT_STATE_ERROR);
        return false;
    }
    obj->state(ULSBUS_OBJECT_STATE_BUSY);
    return true;
}
void ULSBus::sendNM()
{
    _connections.sendNM(_selfDevice->status());
}
bool ULSBus::processNM(ULSBusConnection* pxConnection)
{
    _ulsbus_packet  *pxRxPack = (_ulsbus_packet *)(pxConnection->interface()->rxBufInstance.buf);
    // Interface refresh device timeout
    _connections.refresh(pxConnection,pxRxPack->nm.self_id);
    // Send NM pack to all other interfaces
    _connections.redirect(pxConnection);
    // Update devices
    _ulsbus_device_status status;
    status.devClass = pxRxPack->nm.dev_class;
    status.hardware  = pxRxPack->nm.hardware;
    status.id = pxRxPack->nm.self_id;
    status.status1 = pxRxPack->nm.status1;
    status.status2 = pxRxPack->nm.status2;

    ULSDeviceBase* remoteDevice = _remoteDevices.head(); //m self_id = 0 -> from any devices

    while(remoteDevice){
        if(remoteDevice->status()->devClass == pxRxPack->nm.dev_class)
        {
            if(remoteDevice->id()==pxRxPack->nm.self_id){
                if(!remoteDevice->connected())remoteDevice->connected(true); // Connect device
                //dev->status(&status);
                return true;
            }
        }
        remoteDevice = _remoteDevices.forward(remoteDevice);
    }
    // No defined
    addDevice(&status);
    return true;
}
bool ULSBus::processAck(ULSBusConnection* pxConnection)
{
    _ulsbus_packet  *pxRxPack = (_ulsbus_packet *)(pxConnection->interface()->rxBufInstance.buf);

    uint8_t self_id = pxRxPack->ack.self_id;
    uint8_t remote_id = pxRxPack->ack.remote_id;

    // Redirect if we have connected device
    _connections.redirect(self_id,pxConnection);
    uint8_t ack = (pxRxPack->ack.ackcmd>>5);
    switch(ack){
    case ULSBUS_ACK_OK:{
        // WTF ? Unused Ack received
    }
        break;
    case ULSBUS_ACK_COMPLITE:{

        // All done close transaction
        ULSBusTransaction* pxt = _tarnsactions.find(pxConnection,self_id,remote_id,ULSBUST_TRANSMIT_COMPLITE_WAIT_ACK);
        if(pxt){
            //ULSBUS_LOG("%s: Received ULSBUS_ACK_COMPLITE: self_id: 0x%X remote_id: 0x%X ",_name,self_id,remote_id);
            pxt->processPacket();
            return true;
        }
        //ULSBUS_LOG("%s: UNTRANSACTION ULSBUS_ACK_COMPLITE: self_id: 0x%X remote_id: 0x%X ",_name,self_id,remote_id);
    }
        break;

    case ULSBUS_ACK_OBJECT_ACCESS_DENIED:{
        ULSBUS_LOG("%s: Received ULSBUS_ACK_OBJECT_ACCESS_DENIED: self_id: 0x%X remote_id: 0x%X ",_name,self_id,remote_id);
        // All done close transaction
        ULSBusTransaction* pxt = _tarnsactions.find(pxConnection,self_id,remote_id,ULSBUST_TRANSMIT_COMPLITE_WAIT_ACK);
        if(pxt){
            if(pxt->buffer()->obj())
            {
                pxt->buffer()->obj()->state(ULSBUS_OBJECT_STATE_ERROR);
            }
            pxt->close();
            return true;
        }
    }
        break;
    case ULSBUS_ACK_BUFFER_FULL:
        ULSBUS_LOG("%s: Received ULSBUS_ACK_BUFFER_FULL: self_id: 0x%X remote_id: 0x%X ",_name,self_id,remote_id);
        break;
    case ULSBUS_ACK_OBJECT_NOTFOUND:
        ULSBUS_LOG("%s: Received ULSBUS_ACK_OBJECT_NOTFOUND: self_id: 0x%X remote_id: 0x%X ",_name,self_id,remote_id);
        break;
    case ULSBUS_ACK_OBJECT_SIZE_MISMUTCH:
        ULSBUS_LOG("%s: Received ULSBUS_ACK_OBJECT_SIZE_MISMUTCH: self_id: 0x%X remote_id: 0x%X ",_name,self_id,remote_id);
        break;
    case ULSBUS_ACK_OBJECT_CRC_MISMUTCH:
        ULSBUS_LOG("%s: Received ULSBUS_ACK_OBJECT_CRC_MISMUTCH: self_id: 0x%X remote_id: 0x%X ",_name,self_id,remote_id);
        break;
    default:
        ULSBUS_ERROR("%s: Received Undefine ACK: self_id: 0x%X remote_id: 0x%X ",_name,self_id,remote_id);
        break;
    }
    return true;
}

bool ULSBus::processPacket(ULSBusConnection *pxConnection)
{
    if(!pxConnection)return false; // WTF ?
    _ulsbus_packet  *pxRxPack = (_ulsbus_packet*)(pxConnection->interface()->rxBufInstance.buf);
    uint8_t cmd = pxRxPack->hdr.cmd;//
    uint32_t  packLenght = pxConnection->interface()->rxBufInstance.lenght;

    // Interface refresh device connected timeout
    _connections.refresh(pxConnection,pxRxPack->hdr.self_id);

    switch(cmd){
    case ULSBUS_RTSP://RealTime SYNC Pulse (RTSP)
        // If master need to add do device lists
        _connections.redirect(pxConnection);
        return true; // No need to return data
        break;
    case    ULSBUS_SEC: //System Emergency Command (SEC)
        _connections.redirect(pxConnection);
        return true; // No need to return data
        break;
    case    ULSBUS_NM:{//Network management (NM) - heart beat
        processNM(pxConnection);
        return true; // No need to return data
    }
        break;
    case    ULSBUS_ACK://Operation ACK (ACK)
        return processAck(pxConnection);
        break;
    case    ULSBUS_RLF:
        break;
    case    ULSBUS_BOI_SFT:{  //Broadcast Single Frame Transaction (BOI-SFT)

        uint32_t obj_size = (packLenght - ULSBUS_HEADER_SIZE_BOI_SFT);
        uint8_t  self_id = pxRxPack->boi_sft.self_id;
        uint16_t obj_id = pxRxPack->boi_sft.obj_id;
        uint8_t  *obj_data = pxRxPack->boi_sft.data;
        //ULSBUS_LOG("%s: Received ULSBUS_BOI_SFT: self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,0,obj_id);
        ULSBusObjectBase *obj =  _remoteDevices.getObject(self_id,obj_id); // Looking for object in library
        if(obj){ // check if we found object
            if(obj->size() == obj_size){ // check size match
                obj->setData(obj_data);
            }else {
                // TODO: Error here it not be, mean dictionary not consistent;
            }
            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size,obj);
            if(!buffer) return false;
            // Set data with current object
            if(obj->permition() == ULSBUS_OBJECT_PERMITION_READONLY)
            {
                pxConnection->sendAck(ULSBUS_ACK_OBJECT_ACCESS_DENIED,cmd,self_id,0);
                return false;
            }
            buffer->setData(obj_data,obj_size);
            // Create Transaction for each interface
            ULSBusConnection *px = _connections.head();
            while(px){
                if(px != pxConnection){ // if not current interface
                    ULSBusTransaction* pxt = _tarnsactions.open(px,self_id,0,buffer,ULSBUST_BOI_TRANSMIT_START);
                    if(!pxt){
                        // No ACK
                        return false;
                    }
                }
                px = _connections.forward(px);
            }
        }
        return true;
    }
        break;
    case    ULSBUS_BOI_SOT:{  //Broadcast OI Start Of Transaction (BOI-SOT)

        uint8_t self_id = pxRxPack->boi_sot.self_id;
        uint16_t obj_id = pxRxPack->boi_sot.obj_id;
        uint32_t obj_size = pxRxPack->boi_sot.obj_size;
       // ULSBUS_LOG("%s: Received ULSBUS_BOI_SOT: self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,0,obj_id);

        ULSBusObjectBase *obj =  _remoteDevices.getObject(self_id,obj_id); // Looking for object in library

        // Create Transaction Buffer
        ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size,obj);
        if(!buffer) return false; // No ack for broadcast

        // Create Transaction for current pxc
        ULSBusTransaction* pxt = _tarnsactions.open(pxConnection,self_id,0,buffer,ULSBUST_BOI_RECEIVE_START);
        if(!pxt) return false;// No ack for broadcast
        ULSBusConnection *px = _connections.head();
        while(px){
            if(px != pxConnection){ // if not current interface
                _tarnsactions.open(px,self_id,0,buffer,ULSBUST_BOI_TRANSMIT_START);
            }
            px = _connections.forward(px);
        }
    }
        break;
    case    ULSBUS_BOI_F :{   //Broadcast OI Frame - (BOI-F)
        ULSBusTransaction* pxt = _tarnsactions.find(pxConnection,pxRxPack->boi_f.self_id,0,ULSBUST_BOI_RECEIVE_F);
        if(pxt) pxt->processPacket();
    }
        break;

    case    ULSBUS_RWOI_SFT:{ //Request Write OI Single Frame Transaction (RWOI-SFT)
        uint32_t obj_size = (packLenght - ULSBUS_HEADER_SIZE_RWOI_SFT);
        uint8_t  self_id = pxRxPack->rwoi_sft.self_id;
        uint8_t  remote_id = pxRxPack->rwoi_sft.remote_id;
        uint16_t obj_id = pxRxPack->rwoi_sft.obj_id;
        uint8_t  *obj_data = pxRxPack->rwoi_sft.data;
        //ULSBUS_LOG("%s: Received ULSBUS_RWOI_SFT: self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,remote_id,obj_id);
        if(_selfDevice->id() == remote_id){
            // our device need to write.
            ULSBusObjectBase *obj =  _selfDevice->getObject(obj_id); // Looking for object in library
            if(!obj){
                pxConnection->sendAck(ULSBUS_ACK_OBJECT_NOTFOUND,cmd,self_id,remote_id);
                return false;
            }
            if(obj->size()!= obj_size){
                pxConnection->sendAck(ULSBUS_ACK_OBJECT_SIZE_MISMUTCH,cmd,self_id,remote_id);
                return false;
            }
            if(obj->permition() == ULSBUS_OBJECT_PERMITION_READONLY)
            {
                pxConnection->sendAck(ULSBUS_ACK_OBJECT_ACCESS_DENIED,cmd,self_id,remote_id);
                return false;
            }
            obj->setData(obj_data);
            //ULSBUS_LOG("%s: OBJECT [0x%X] Received : self_id: 0x%X remote_id: 0x%X ",pxConnection->interface()->name(),obj_id,self_id,remote_id);
            pxConnection->sendAck(ULSBUS_ACK_COMPLITE,cmd,self_id,remote_id);
            obj->state(ULSBUS_OBJECT_STATE_OK);
            return true;


        }else{
            // check if we have devices connected to us with corresponding ID
            ULSBusConnection* pxc =  _connections.findId(remote_id);
            if(!pxc){return true;} //  connection not found

            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size,__null);
            if(!buffer){
                pxConnection->sendAck(ULSBUS_ACK_BUFFER_FULL,cmd,self_id,remote_id);
                return false;
            }
            buffer->setData(obj_data,obj_size);
            ULSBusTransaction* pxt = _tarnsactions.open(pxc,self_id,remote_id,buffer,ULSBUST_RWOI_TRANSMIT_START);
            if(!pxt){
                pxConnection->sendAck(ULSBUS_ACK_BUFFER_FULL,cmd,self_id,remote_id);
                return false;
            }
            return true;
        }

    }
        break;
    case    ULSBUS_RWOI_SOT:{ //Request Write OI SOT (RWOI-SOT)
        uint8_t self_id = pxRxPack->rwoi_sot.self_id;
        uint8_t remote_id = pxRxPack->rwoi_sot.remote_id;
        uint16_t obj_id = pxRxPack->rwoi_sot.obj_id;
        uint16_t obj_size = pxRxPack->rwoi_sot.obj_size;
        //ULSBUS_LOG("%s: Received ULSBUS_RWOI_SOT: self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,remote_id,obj_id);
        if(_selfDevice->id() == remote_id){
            ULSBusObjectBase *obj =  _selfDevice->getObject(obj_id); // Looking for object in library
            if(!obj){
                pxConnection->sendAck(ULSBUS_ACK_OBJECT_NOTFOUND,cmd,self_id,remote_id);
                return false;
            }
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size,obj);
            if(!buffer){
                pxConnection->sendAck(ULSBUS_ACK_BUFFER_FULL,cmd,self_id,remote_id);
                return false;
            }
            ULSBusTransaction* pxt = _tarnsactions.open(pxConnection,self_id,remote_id,buffer,ULSBUST_RWOI_RECEIVE_START);
            if(!pxt){
                pxConnection->sendAck(ULSBUS_ACK_BUFFER_FULL,cmd,self_id,remote_id);
                return false;
            }
            return true;
        }else{
            //ULSBUS_LOG("%s: ULSBUS_RWOI_SOT - RETRANSMIT : self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,remote_id,obj_id);
            // Redirect to other devices;
            ULSBusConnection* pxc =  _connections.findId(remote_id);
            if(!pxc) return false; // connection with device not found

            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size,__null);
            if(!buffer){
                pxConnection->sendAck(ULSBUS_ACK_BUFFER_FULL,cmd,self_id,remote_id);
                return false;
            }
            // Create Transaction for current interface to receive data
            ULSBusTransaction* pxt_receive = _tarnsactions.open(pxConnection,self_id,remote_id,buffer,ULSBUST_RWOI_RECEIVE_START);
            if(!pxt_receive){
                pxConnection->sendAck(ULSBUS_ACK_BUFFER_FULL,cmd,self_id,remote_id);
                return false;
            }
            // create transction to transmit data
            ULSBusTransaction* pxt_transmit = _tarnsactions.open(pxc,self_id,remote_id,buffer,ULSBUST_RWOI_TRANSMIT_START);
            if(!pxt_transmit){
                pxConnection->sendAck(ULSBUS_ACK_BUFFER_FULL,cmd,self_id,remote_id);
                pxt_receive->close();
                return false;
            }
            return true;
        }
    }
        break;
    case    ULSBUS_RWOI_F:{   //Request Write OI Frame (RWOI-F)
        // Find receiving transaction
        ULSBusTransaction* pxt = _tarnsactions.find(pxConnection,pxRxPack->aoi_f.self_id,pxRxPack->aoi_f.remote_id,ULSBUST_RWOI_RECEIVE_F);
        if(pxt) pxt->processPacket();
    }
        break;
    case    ULSBUS_RROI:{     //Request Read Object Instance (RROI)
        uint8_t self_id = pxRxPack->rroi.self_id;
        uint8_t remote_id = pxRxPack->rroi.remote_id;
        uint16_t obj_id = pxRxPack->rroi.obj_id;
        uint16_t obj_size = pxRxPack->rroi.size;

        //ULSBUS_LOG("%s: Received ULSBUS_RROI: self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,remote_id,obj_id);
        if(_selfDevice->id() == remote_id){

            ULSBusObjectBase *obj =  _selfDevice->getObject(obj_id); // Looking for object in library
            if(!obj){
                pxConnection->sendAck(ULSBUS_ACK_OBJECT_NOTFOUND,cmd,self_id,remote_id);
                return false;
            }
            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size,obj);
            if(!buffer){
                pxConnection->sendAck(ULSBUS_ACK_BUFFER_FULL,cmd,self_id,remote_id);
                return false;
            }
            buffer->setData(obj->data(),obj->size());
            // Create Transaction for current interface to transmit data
            ULSBusTransaction* pxt = _tarnsactions.open(pxConnection,self_id,remote_id,buffer,ULSBUST_AOI_TRANSMIT_START);
            if(!pxt){
                pxConnection->sendAck(ULSBUS_ACK_BUFFER_FULL,cmd,self_id,remote_id);
                return false;
            }
            return true;

        }else{
           // ULSBUS_LOG("%s: ULSBUS_RROI - RETRANSMIT : self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,remote_id,obj_id);
            // Redirect to other devices;
            ULSBusConnection* pxc =  _connections.findId(remote_id);
            if(!pxc) return true; // connection with device not found
            pxc->send(&(pxConnection->interface()->rxBufInstance));
        }
    }
        break;
    case    ULSBUS_AOI_SFT:{  //Answer OI SOT (AIO-SOT)
        uint8_t self_id = pxRxPack->aoi_sft.self_id;
        uint8_t remote_id = pxRxPack->aoi_sft.remote_id;
        uint16_t obj_id = pxRxPack->aoi_sft.obj_id;
        uint16_t obj_size = (packLenght - ULSBUS_HEADER_SIZE_AOI_SFT);
        uint8_t* obj_data = pxRxPack->aoi_sft.data;
        //ULSBUS_LOG("%s: Received ULSBUS_AOI_SFT: self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,remote_id,obj_id);

        ULSDeviceBase *remoteDevice = _remoteDevices.findDevice(remote_id);
        if(remoteDevice){
            ULSBusObjectBase *obj =  remoteDevice->getObject(obj_id); // remote_id = 0 -> from any devices
            if(!obj)return false; // No ack if device not found
            obj->setData(obj_data);
            obj->state(ULSBUS_OBJECT_STATE_OK);
            //ULSBUS_LOG("%s: OBJECT [0x%X] Received : self_id: 0x%X remote_id: 0x%X ",pxConnection->interface()->name(),obj_id,self_id,remote_id);
            return true;
        }else{
           // ULSBUS_LOG("%s: ULSBUS_AOI_SFT - RETRANSMIT : self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,remote_id,obj_id);
            // Redirect to other devices;
            ULSBusConnection* pxc =  _connections.findId(self_id);
            if(!pxc) return true; // connection with device not found

            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size,__null);
            if(!buffer){
                //NO ACK for reading command
                return false;
            }
            buffer->setData(obj_data,obj_size);
            // create transction to transmit data
            ULSBusTransaction* pxt = _tarnsactions.open(pxc,self_id,remote_id,buffer,ULSBUST_AOI_TRANSMIT_START);
            if(!pxt){
                //NO ACK for reading command
                return false;
            }
        }
    }
    case    ULSBUS_AOI_SOT:{  //Answer OI SOT (AIO-SOT)
        uint8_t self_id = pxRxPack->aoi_sot.self_id;
        uint8_t remote_id = pxRxPack->aoi_sot.remote_id;
        uint16_t obj_id = pxRxPack->aoi_sot.obj_id;
        uint16_t obj_size = pxRxPack->aoi_sot.size;

        //ULSBUS_LOG("%s: Received ULSBUS_AOI_SOT: self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,remote_id,obj_id);
        ULSDeviceBase *remoteDevice = _remoteDevices.findDevice(remote_id);
        if(remoteDevice){
            ULSBusObjectBase *obj =  remoteDevice->getObject(obj_id); // Looking for object in library
            if(!obj){
                return false; // WTF ?
            }
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size,obj);
            if(!buffer)return false; //WTF?
            // Create Transaction for current interface
            ULSBusTransaction* pxt = _tarnsactions.open(pxConnection,self_id,remote_id,buffer,ULSBUST_AOI_RECEIVE_START);
            if(!pxt)return false;
            return true;

        }else{
            //ULSBUS_LOG("%s: ULSBUS_AOI_SOT - RETRANSMIT : self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,remote_id,obj_id);

            // Redirect to other devices;
            ULSBusConnection* pxc =  _connections.findId(self_id);
            if(!pxc)
            {
                //ULSBUS_LOG("%s: ULSBUS_AOI_SOT - No Connected Devices : self_id: 0x%X remote_id: 0x%X object: 0x%X",_name,self_id,remote_id,obj_id);

                return false; // pxc with device not found
            }
            // Redirect request to next device;
            // No redirect wi will aspxc->send(&(pxConnection->interface()->rxBufInstance));
            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size,__null);
            if(!buffer){
                //NO ACK for reading command
                return false;
            }
            // Create Transaction for current interface to receive data
            ULSBusTransaction* pxt = _tarnsactions.open(pxConnection,self_id,remote_id,buffer,ULSBUST_AOI_RECEIVE_START);
            if(!pxt){
                //NO ACK for reading command
                return false;
            }
            // create transction to transmit data
            pxt = _tarnsactions.open(pxc,self_id,remote_id,buffer,ULSBUST_AOI_TRANSMIT_START);
            if(!pxt){
                //NO ACK for reading command
                return false;
            }
            return true;
        }


    }
    case    ULSBUS_AOI_F: {   //Answer OI frame (AIO-F)
        ULSBusTransaction* pxt = _tarnsactions.find(pxConnection,pxRxPack->aoi_f.self_id,pxRxPack->aoi_f.remote_id,ULSBUST_AOI_RECEIVE_F);
        if(pxt) pxt->processPacket();
    }
        break;
        //case    ULSBUS_AOI_C:    //Answer OI complete (AIO-C)
    }
    return false;

}
uint32_t ULSBus::openedTransactions()
{
    return _tarnsactions.openedTransactions();
}
void ULSBus::add(ULSBusTransaction* transaction)
{
    //   transaction->library(&_remoteDevices);
    _tarnsactions.add(transaction);
}
void ULSBus::add(ULSBusConnection* pxc)
{
    pxc->interface()->enableEscIfSupprted(true);
    _connections.add(pxc);
}
void ULSBus::add(ULSBusObjectBuffer* buf)
{
    _oBuf.add(buf);
}
void ULSBus::add(ULSBusTransaction* transaction, uint32_t len)
{
    while(len--){
        add(transaction);
        transaction++;
    }
}
void ULSBus::add(ULSBusConnection* pxc, uint32_t len)
{
    while(len--){
        _connections.add(pxc);
        pxc++;
    }
}
void ULSBus::add(ULSBusObjectBuffer* buf, uint32_t len)
{
    while(len--){
        _oBuf.add(buf);
        buf++;
    }
}
void ULSBus::add(ULSDeviceBase* device)
{

    _remoteDevices.add(device);
}
void ULSBus::updatedCallback(_ulsbus_obj_updated_callback callback)
{

    _remoteDevices.updatedCallback(callback);
}
