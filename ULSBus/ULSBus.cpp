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

ULSBus::ULSBus():
    _nmTimeout(0)
{
    _tarnsactions.library(&_library);
};
void ULSBus::task()// call every ms
{
    _nmTimeout++;
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
void ULSBus::sendNM()
{
    ULSDeviceBase *dev = _library.head();
    while(dev)
    {
        if((dev->remote_id() == 0)){ // remote_id == 0 for all current devices
            _connections.sendNM(dev->status());
        }
        dev = _library.forward(dev);
    }
}

bool ULSBus::processAck(ULSBusConnection* pxConnection)
{
    _ulsbus_packet  *pxPack = (_ulsbus_packet *)(pxConnection->interface()->rxBufInstance.buf);

    uint8_t self_id = pxPack->ack.self_id;
    uint8_t remote_id = pxPack->ack.remote_id;

    // Redirect if it is not our device;
    if(!_library.findDevices(remote_id,0))
    {
        _connections.redirect(pxConnection);
    }

    switch(pxPack->ack.ackcmd){
    case ULSBUS_ACK_RWOI_SFT_OK:{

    }
        break;
    case ULSBUS_ACK_RWOI_SFT_OBJECT_NOTFOUND:{

    }
        break;
    case ULSBUS_ACK_RWOI_SFT_OBJECT_SIZE_MISNUCH:{

    }
        break;
    case ULSBUS_ACK_RWOI_SFT_OBJECT_CRC_MISNUCH:{

    }
        break;
    case ULSBUS_ACK_RWOI_SFT_OBJECT_BUFFER_FULL:{

    }
        break;
    case ULSBUS_ACK_RWOI_SOT_OK:{

    }
        break;
    case ULSBUS_ACK_RWOI_SOT_OBJECT_NOTFOUND:{

    }
        break;
    case ULSBUS_ACK_RWOI_SOT_OBJECT_SIZE_MISMUCH:{

    }
        break;
    case ULSBUS_ACK_RWOI_SOT_OBJECT_CRC_MISMUCH:{

    }
        break;
    case ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL:{

    }
        break;
    case ULSBUS_ACK_RROI_OK:{

    }
        break;
    case ULSBUS_ACK_RROI_OBJECT_NOTFOUND:{

    }
        break;
    case ULSBUS_ACK_RROI_OBJECT_SIZE_MISMUCH:{

    }
        break;
    case USBUS_ACK_AOI_SOT_COMPLITE:
    case USBUS_ACK_AOI_SFT_COMPLITE:{
        // All done close transaction
        ULSBusTransaction* pxt = _tarnsactions.find(pxConnection,self_id,remote_id,ULSBUST_AOI_TRANSMIT_COMPLITE_WAIT_ACK);
        if(pxt)pxt->close();
    }
        break;

    default:
        break;
    }

    return true;
}

bool ULSBus::processPacket(ULSBusConnection *pxConnection)
{
    if(!pxConnection)return false; // WTF ?
    _ulsbus_packet  *pxPack = (_ulsbus_packet*)(pxConnection->interface()->rxBufInstance.buf);
    uint8_t cmd = pxPack->hdr.cmd;//
    uint32_t  packLenght = pxConnection->interface()->rxBufInstance.lenght;

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
        // Interface refresh device timeout
        _connections.refresh(pxConnection,pxPack->nm.self_id);
        // Send NM pack to all other interfaces
        _connections.redirect(pxConnection);
        // Update devices

        ULSDeviceBase* dev = _library.head(); //m self_id = 0 -> from any devices
        while(dev){
            if(dev->status()->dev_class == pxPack->nm.dev_class)
            {
                if(dev->remote_id()==pxPack->nm.self_id){
                    if(!dev->connected())dev->connected(true); // Connect device
                    // Device found - all ok
                    return true;
                }
            }
            dev = _library.forward(dev);
        }
        // No defined
        addDevice(pxPack->nm.self_id,pxPack->nm.dev_class);

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
        uint8_t  self_id = pxPack->boi_sft.self_id;
        uint16_t obj_id = pxPack->boi_sft.obj_id;
        uint8_t  *obj_data = pxPack->boi_sft.data;

        ULSBusObjectBase *obj =  _library.getObject(0x0,self_id,obj_id); // Looking for object in library
        if(obj){ // check if we found object
            if(obj->size() == obj_size){ // check size match
                obj->setData(obj_data);
            }else {
                // TODO: Error here it not be, mean dictionary not consistent;
            }
            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
            if(!buffer) return false;
            // Set data with current object
            buffer->setData(obj_data,obj_size);
            // Create Transaction for each interface
            ULSBusConnection *px = _connections.head();
            while(px){
                if(px != pxConnection){ // if not current interface
                    ULSBusTransaction* pxt = _tarnsactions.open(px,pxConnection,self_id,0,buffer,ULSBUST_BOI_TRANSMIT_START);
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

        uint8_t self_id = pxPack->boi_sot.self_id;
        uint16_t obj_id = pxPack->boi_sot.obj_id;
        uint32_t obj_size = pxPack->boi_sot.size;

        // Create Transaction Buffer
        ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);

        // Create Transaction for each interface
        if(!buffer) return false; // No ack for broadcast

        // Create Transaction for current pxc
        ULSBusTransaction* pxt = _tarnsactions.open(pxConnection,self_id,0,buffer,ULSBUST_BOI_RECEIVE_START);
        if(!pxt) return false;// No ack for broadcast
        ULSBusConnection *px = _connections.head();
        while(px){
            if(px != pxConnection){ // if not current interface
                pxt = _tarnsactions.open(px,pxConnection,self_id,0,buffer,ULSBUST_BOI_TRANSMIT_START);
                if(!pxt){
                    // No ACK
                    return false;
                }
            }
            px = _connections.forward(px);
        }
    }
        break;
    case    ULSBUS_BOI_F :{   //Broadcast OI Frame - (BOI-F)
        ULSBusTransaction* pxt = _tarnsactions.find(pxConnection,pxPack->boi_f.self_id,0,ULSBUST_BOI_RECEIVE_F);
        if(pxt) pxt->processPacket();
    }
        break;

    case    ULSBUS_RWOI_SFT:{ //Request Write OI Single Frame Transaction (RWOI-SFT)
        uint32_t obj_size = (packLenght - ULSBUS_HEADER_SIZE_RWOI_SFT);
        uint8_t self_id = pxPack->rwoi_sft.self_id;
        uint8_t remote_id = pxPack->rwoi_sft.remote_id;
        uint16_t obj_id = pxPack->rwoi_sft.obj_id;
        uint8_t  *obj_data = pxPack->rwoi_sft.data;

        _ulsbus_obj_find_rezult rez = _library.find(remote_id,0,obj_id,obj_size); //m self_id = 0 -> from any devices

        if(rez == ULSBUS_OBJECT_FIND_OK){
            ULSBusObjectBase *obj =  _library.getObject(remote_id,0,obj_id); // Looking for object in library
            // check if we found object
            if(!obj){return false;}
            obj->setData(obj_data);
            // Send Ack - we have received data
            pxConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OK,self_id,remote_id);
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND){
            pxConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_NOTFOUND,self_id,remote_id);
            return false;
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH){
            pxConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_SIZE_MISNUCH,self_id,remote_id);
            return false;
        }
        if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){
            // check if we have devices connected to us with corresponding ID
            ULSBusConnection* pxc =  _connections.findId(remote_id);
            if(!pxc){return true;} //  connection not found

            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
            if(!buffer){
                pxConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_BUFFER_FULL,self_id,remote_id);
                return false;
            }
            // fill buffer with received data
            buffer->setData(obj_data,obj_size);
            ULSBusTransaction* pxt = _tarnsactions.open(pxc,pxConnection,self_id,remote_id,buffer,ULSBUST_RWOI_TRANSMIT_START);
            if(!pxt){
                pxConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_BUFFER_FULL,self_id,remote_id);
                return false;
            }
            return true;
        }
    }
        break;
    case    ULSBUS_RWOI_SOT:{ //Request Write OI SOT (RWOI-SOT)
        uint8_t self_id = pxPack->rwoi_sot.self_id;
        uint8_t remote_id = pxPack->rwoi_sot.remote_id;
        uint16_t obj_id = pxPack->rwoi_sot.obj_id;
        uint16_t obj_size = pxPack->rwoi_sot.size;

        _ulsbus_obj_find_rezult rez = _library.find(remote_id,self_id,obj_id,obj_size); //m self_id = 0 -> from any devices
        if(rez == ULSBUS_OBJECT_FIND_OK){
            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
            if(!buffer){
                pxConnection->sendAck(ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL,self_id,remote_id);
                return false;
            }
            // Create Transaction for current interface
            ULSBusTransaction* pxt = _tarnsactions.open(pxConnection,0,remote_id,buffer,ULSBUST_RWOI_RECEIVE_START);
            if(!pxt){
                pxConnection->sendAck(ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL,self_id,remote_id);
                return false;
            }
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){
            // Redirect to other devices;
            ULSBusConnection* pxc =  _connections.findId(remote_id);
            if(!pxc) return false; // connection with device not found
            // Redirect request to next device;
            pxc->send(&(pxConnection->interface()->rxBufInstance));
            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
            if(!buffer){
                pxConnection->sendAck(ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL,self_id,remote_id);
                return false;
            }
            // Create Transaction for current interface to receive data
            ULSBusTransaction* pxt = _tarnsactions.open(pxc,pxConnection,self_id,remote_id,buffer,ULSBUST_RWOI_RECEIVE_START);
            if(!pxt){
                pxConnection->sendAck(ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL,self_id,remote_id);
                return false;
            }
            // create transction to transmit data
            pxt = _tarnsactions.open(pxc,self_id,remote_id,buffer,ULSBUST_RWOI_TRANSMIT_START);
            if(!pxt){
                pxConnection->sendAck(ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL,self_id,remote_id);
                return false;
            }
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND){
            pxConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_NOTFOUND,self_id,remote_id);
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH){
            pxConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_NOTFOUND,self_id,remote_id);
            return true;
        }

    }
        break;
    case    ULSBUS_RWOI_F:{   //Request Write OI Frame (RWOI-F)
        // Find receiving transaction
        ULSBusTransaction* pxt = _tarnsactions.find(pxConnection,pxPack->aoi_f.self_id,pxPack->aoi_f.remote_id,ULSBUST_RWOI_RECEIVE_F);
        if(pxt) pxt->processPacket();
    }
        break;
    case    ULSBUS_RROI:{     //Request Read Object Instance (RROI)
        uint8_t self_id = pxPack->rroi.self_id;
        uint8_t remote_id = pxPack->rroi.remote_id;
        uint16_t obj_id = pxPack->rroi.obj_id;
        uint16_t obj_size = pxPack->rroi.size;

        _ulsbus_obj_find_rezult rez = _library.find(remote_id,0,obj_id,obj_size);

        if(rez == ULSBUS_OBJECT_FIND_OK){
            // Create Transaction Buffer

            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);

            if(!buffer){
                pxConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_SIZE_MISMUCH,self_id,remote_id);
                return false;
            }

            // Fill buffer with data.
            ULSBusObjectBase *obj =  _library.getObject(remote_id,0,obj_id); // Looking for object in library
            // check if we found object
            if(!obj)return false;
            buffer->setData(obj->data(),obj->size());
            // Create Transaction for current interface to transmit data
            ULSBusTransaction* pxt = _tarnsactions.open(pxConnection,self_id,remote_id,buffer,ULSBUST_AOI_TRANSMIT_START);
            if(!pxt){
                pxConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_BUFFER_FULL,self_id,remote_id);
                return false;
            }
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){

            // Redirect to other devices;
            ULSBusConnection* pxc =  _connections.findId(remote_id);
            if(!pxc) return true; // connection with device not found
            // Redirect request to next device;
            pxc->send(&(pxConnection->interface()->rxBufInstance));
            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
            if(!buffer){
                pxConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_BUFFER_FULL,self_id,remote_id);
                return false;
            }
            // Create Transaction for current interface to transmit data
            ULSBusTransaction* pxt = _tarnsactions.open(pxConnection,self_id,remote_id,buffer,ULSBUST_AOI_TRANSMIT_START);
            if(!pxt){
                pxConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_BUFFER_FULL,self_id,remote_id);
                return false;
            }
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND){
            pxConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_NOTFOUND,self_id,remote_id);
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH){
            pxConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_SIZE_MISMUCH,self_id,remote_id);
            return true;
        }
    }
        break;
    case    ULSBUS_AOI_SFT:{  //Answer OI SOT (AIO-SOT)
        uint8_t self_id = pxPack->aoi_sft.self_id;
        uint8_t remote_id = pxPack->aoi_sft.remote_id;
        uint16_t obj_id = pxPack->aoi_sft.obj_id;
        uint16_t obj_size = (packLenght - ULSBUS_HEADER_SIZE_AOI_SFT);
        uint8_t* obj_data = pxPack->aoi_sft.data;
        _ulsbus_obj_find_rezult rez = _library.find(self_id,0,obj_id,obj_size); //remote_id = 0 -> from any devices

        if(rez == ULSBUS_OBJECT_FIND_OK){
            // Create Transaction Buffer
            // Fill buffer with data.
            ULSBusObjectBase *obj =  _library.getObject(self_id,0,obj_id); // remote_id = 0 -> from any devices
            // check if we found object
            if(!obj)return false;
            obj->setData(obj_data);
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){
            // Redirect to other devices;
            ULSBusConnection* pxc =  _connections.findId(remote_id);
            if(!pxc) return true; // connection with device not found
            ULSBusTransaction* pxt = _tarnsactions.find(pxc,self_id,remote_id,ULSBUST_AOI_RECEIVE_F);
            if(!pxt)return false; // To transaction to redirect
            ULSBusObjectBuffer* buffer = pxt->buffer();
            if(!buffer)return false; // WTF?
            buffer->setData(obj_data,obj_size);
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND){
            //NO ACK for reading command
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH){
            //NO ACK for reading command
            return true;
        }
    }
    case    ULSBUS_AOI_SOT:{  //Answer OI SOT (AIO-SOT)
        uint8_t self_id = pxPack->aoi_sot.self_id;
        uint8_t remote_id = pxPack->aoi_sot.remote_id;
        uint16_t obj_id = pxPack->aoi_sot.obj_id;
        uint16_t obj_size = pxPack->aoi_sot.size;

        _ulsbus_obj_find_rezult rez = _library.find(remote_id,0,obj_id,obj_size); //m self_id = 0 -> from any devices

        if(rez == ULSBUS_OBJECT_FIND_OK){
            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
            if(!buffer){
                //NO ACK for reading command
                return false;
            }
            // Create Transaction for current interface
            ULSBusTransaction* pxt = _tarnsactions.open(pxConnection,self_id,remote_id,buffer,ULSBUST_AOI_RECEIVE_START);
            if(!pxt){
                //NO ACK for reading command
                return false;
            }
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){
            // Redirect to other devices;
            ULSBusConnection* pxc =  _connections.findId(remote_id);
            if(!pxc) return false; // pxc with device not found
            // Redirect request to next device;
            pxc->send(&(pxConnection->interface()->rxBufInstance));
            // Create Transaction Buffer
            ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
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
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND){
            //NO ACK for reading command
            return true;
        }
        if(rez == ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH){
            //NO ACK for reading command
            return true;
        }
    }
    case    ULSBUS_AOI_F: {   //Answer OI frame (AIO-F)
        ULSBusTransaction* pxt = _tarnsactions.find(pxConnection,pxPack->aoi_f.self_id,pxPack->aoi_f.remote_id,ULSBUST_AOI_RECEIVE_F);
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
        _tarnsactions.add(transaction);
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

    _library.add(device);
}
