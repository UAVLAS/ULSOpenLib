#ifndef ULSBUS_H
#define ULSBUS_H

#include <string.h>
#include "ULSBusTypes.h"
#include "ULSBusTransaction.h"
#include "ULSBusConnection.h"
#include "ULSBusObject.h"

template<int TP_NUM>
class ULSBus
{
public:
    ULSBus(ULSBusObjectsLibrary *library):_library(library)
    {
        _tarnsactions.library(_library);
    };
    void task()// call every ms
    {
        _nmTimeout++;
        _connections.task();
        _tarnsactions.task();
        for(uint32_t i=0;i<_connections.connectionsNum();i++){
           ULSBusConnection *pxConnection = _connections.connection(i);
            if(pxConnection){
                if( pxConnection->read() != 0){
                processPacket(pxConnection);
                }

            }
        }
        // Send NM packet
        if(_nmTimeout >= ULSBUS_NM_INTERVAL){
            sendNM();
            _nmTimeout = 0;
        }

    }
    void addInterface(IfBase* pxIf,uint32_t frameSize)
    {
        _connections.add(pxIf,frameSize);
    }
    void open()
    {

    }
    void sendNM()
    {
        ULSBusObjectsDictionary *dic = _library->head();
        while(dic)
        {
            if((dic->remote_id() == 0)){ // remote_id == 0 for all current devices
                _connections.sendNM(dic->devStatus());
            }
            dic = dic->next;
        }
    }

    bool processAck(ULSBusConnection* pxConnection)
    {
        _ulsbus_packet  *pxPack = (_ulsbus_packet *)(pxConnection->interface()->rxBufInstance.buf);

//        uint8_t self_id = pxPack->ack.self_id;
//        uint8_t remote_id = pxPack->ack.remote_id;


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

        default:
            break;
        }

        return true;
    }

    bool processPacket(ULSBusConnection *pxConnection)
    {
        _ulsbus_packet  *pxPack = (_ulsbus_packet *)(pxConnection->interface()->rxBufInstance.buf);
        ULSBusConnection* currentConnection = pxConnection;
        if(!currentConnection)return false; // WTF ?
        uint8_t cmd = pxPack->hdr.cmd;//
        uint32_t  packLenght = pxConnection->interface()->rxBufInstance.lenght;

        switch(cmd){
        case ULSBUS_RTSP://RealTime SYNC Pulse (RTSP)
            // If master need to add do device lists
            _connections.redirect(currentConnection);
            return true; // No need to return data
            break;
        case    ULSBUS_SEC: //System Emergency Command (SEC)
            _connections.redirect(currentConnection);
            return true; // No need to return data
            break;
        case    ULSBUS_NM://Network management (NM) - heart beat
            // Interface refresh device timeout
            _connections.refresh(currentConnection,pxPack->nm.self_id);
            // Send NM pack to all other interfaces
            _connections.redirect(currentConnection);
            return true; // No need to return data
            break;
        case    ULSBUS_ACK://Operation ACK (ACK)
            return processAck(currentConnection);
            break;
        case    ULSBUS_RLF:
            break;
        case    ULSBUS_BOI_SFT:{  //Broadcast Single Frame Transaction (BOI-SFT)

            uint32_t obj_size = (packLenght - ULSBUS_HEADER_SIZE_BOI_SFT);
            uint8_t  self_id = pxPack->boi_sft.self_id;
            uint16_t obj_id = pxPack->boi_sft.obj_id;
            uint8_t  *obj_data = pxPack->boi_sft.data;

            ULSBusObjectBase *obj =  _library->getObject(0x0,self_id,obj_id); // Looking for object in library
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
                for(uint32_t i=0;i<_connections.connectionsNum();i++)
                {
                    ULSBusConnection* connection =  _connections.connection(i);
                    if(!connection) return true; // all connections processed
                    if(connection != currentConnection){ // if not current interface
                        ULSBusTransaction* transaction = _tarnsactions.open(connection,currentConnection,self_id,0,buffer,ULSBUST_BOI_TRANSMIT_START);
                        if(!transaction){
                            // No ACK
                            return false;
                        }
                    }
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

            // Create Transaction for current connection
            ULSBusTransaction* transaction = _tarnsactions.open(currentConnection,self_id,0,buffer,ULSBUST_BOI_RECEIVE_START);
            if(!transaction) return false;// No ack for broadcast

            for(int i=0;i<8;i++)
            {
                ULSBusConnection* connection =  _connections.connection(i);
                if(!connection) return true; // all connections processed
                if(connection != currentConnection){ // if not current interface
                    ULSBusTransaction* transaction = _tarnsactions.open(connection,currentConnection,self_id,0,buffer,ULSBUST_BOI_TRANSMIT_START);
                    if(!transaction) return false;// No ack for broadcast
                }

            }
        }
            break;
        case    ULSBUS_BOI_F :{   //Broadcast OI Frame - (BOI-F)
            ULSBusTransaction* transaction = _tarnsactions.find(currentConnection,pxPack->boi_f.self_id,0,ULSBUST_BOI_RECEIVE_F);
            if(transaction) transaction->processPacket();
        }
            break;

        case    ULSBUS_RWOI_SFT:{ //Request Write OI Single Frame Transaction (RWOI-SFT)
            uint32_t obj_size = (packLenght - ULSBUS_HEADER_SIZE_RWOI_SFT);
            uint8_t self_id = pxPack->rwoi_sft.self_id;
            uint8_t remote_id = pxPack->rwoi_sft.remote_id;
            uint16_t obj_id = pxPack->rwoi_sft.obj_id;
            uint8_t  *obj_data = pxPack->rwoi_sft.data;

            _ulsbus_obj_find_rezult rez = _library->find(remote_id,0,obj_id,obj_size); //m self_id = 0 -> from any devices

            if(rez == ULSBUS_OBJECT_FIND_OK){
                ULSBusObjectBase *obj =  _library->getObject(remote_id,0,obj_id); // Looking for object in library
                // check if we found object
                if(!obj)return false;
                obj->setData(obj_data);
                // Send Ack - we have received data
                currentConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OK,self_id,remote_id);
                return true;
            }
            if(rez == ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND){
                currentConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_NOTFOUND,self_id,remote_id);
                return false;
            }
            if(rez == ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH){
                currentConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_SIZE_MISNUCH,self_id,remote_id);
                return false;
            }
            if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){
                // check if we have devices connected to us with corresponding ID
                ULSBusConnection* connection =  _connections.findId(remote_id);
                if(!connection) return true; //  connection not found

                // Create Transaction Buffer
                ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
                if(!buffer){
                    currentConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_BUFFER_FULL,self_id,remote_id);
                    return false;
                }
                // fill buffer with received data
                buffer->setData(obj_data,obj_size);
                ULSBusTransaction* transaction = _tarnsactions.open(connection,currentConnection,self_id,remote_id,buffer,ULSBUST_RWOI_TRANSMIT_START);
                if(!transaction){
                    currentConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_BUFFER_FULL,self_id,remote_id);
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

            _ulsbus_obj_find_rezult rez = _library->find(remote_id,self_id,obj_id,obj_size); //m self_id = 0 -> from any devices
            if(rez == ULSBUS_OBJECT_FIND_OK){
                // Create Transaction Buffer
                ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
                if(!buffer){
                    currentConnection->sendAck(ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL,self_id,remote_id);
                    return false;
                }
                // Create Transaction for current interface
                ULSBusTransaction* transaction = _tarnsactions.open(currentConnection,0,remote_id,buffer,ULSBUST_RWOI_RECEIVE_START);
                if(!transaction){
                    currentConnection->sendAck(ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL,self_id,remote_id);
                    return false;
                }
                return true;
            }
            if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){
                // Redirect to other devices;
                ULSBusConnection* connection =  _connections.findId(remote_id);
                if(!connection) return false; // connection with device not found
                // Redirect request to next device;
                connection->send(&(currentConnection->interface()->rxBufInstance));
                // Create Transaction Buffer
                ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
                if(!buffer){
                    currentConnection->sendAck(ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL,self_id,remote_id);
                    return false;
                }
                // Create Transaction for current interface to receive data
                ULSBusTransaction* transaction = _tarnsactions.open(currentConnection,currentConnection,self_id,remote_id,buffer,ULSBUST_RWOI_RECEIVE_START);
                if(!transaction){
                    currentConnection->sendAck(ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL,self_id,remote_id);
                    return false;
                }
                // create transction to transmit data
                transaction = _tarnsactions.open(connection,self_id,remote_id,buffer,ULSBUST_RWOI_TRANSMIT_START);
                if(!transaction){
                    currentConnection->sendAck(ULSBUS_ACK_RWOI_SOT_OBJECT_BUFFER_FULL,self_id,remote_id);
                    return false;
                }
                return true;
            }
            if(rez == ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND){
                currentConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_NOTFOUND,self_id,remote_id);
                return true;
            }
            if(rez == ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH){
                currentConnection->sendAck(ULSBUS_ACK_RWOI_SFT_OBJECT_NOTFOUND,self_id,remote_id);
                return true;
            }

        }
            break;
        case    ULSBUS_RWOI_F:{   //Request Write OI Frame (RWOI-F)
            // Find receiving transaction
            ULSBusTransaction* transaction = _tarnsactions.find(currentConnection,pxPack->aoi_f.self_id,pxPack->aoi_f.remote_id,ULSBUST_RWOI_RECEIVE_F);
            if(transaction) transaction->processPacket();
        }
            break;
        case    ULSBUS_RROI:{     //Request Read Object Instance (RROI)
            uint8_t self_id = pxPack->rroi.self_id;
            uint8_t remote_id = pxPack->rroi.remote_id;
            uint16_t obj_id = pxPack->rroi.obj_id;
            uint16_t obj_size = pxPack->rroi.size;

            _ulsbus_obj_find_rezult rez = _library->find(remote_id,0,obj_id,obj_size);

            if(rez == ULSBUS_OBJECT_FIND_OK){
                // Create Transaction Buffer

                ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);

                if(!buffer){
                    currentConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_SIZE_MISMUCH,self_id,remote_id);
                    return false;
                }

                // Fill buffer with data.
                ULSBusObjectBase *obj =  _library->getObject(remote_id,0,obj_id); // Looking for object in library
                // check if we found object
                if(!obj)return false;
                buffer->setData(obj->data(),obj->size());
                // Create Transaction for current interface to transmit data
                ULSBusTransaction* transaction = _tarnsactions.open(currentConnection,self_id,remote_id,buffer,ULSBUST_AOI_TRANSMIT_START);
                if(!transaction){
                    currentConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_BUFFER_FULL,self_id,remote_id);
                    return false;
                }
                return true;
            }
            if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){

                // Redirect to other devices;
                ULSBusConnection* connection =  _connections.findId(remote_id);
                if(!connection) return true; // connection with device not found
                // Redirect request to next device;
                connection->send(&(currentConnection->interface()->rxBufInstance));
                // Create Transaction Buffer
                ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
                if(!buffer){
                    currentConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_BUFFER_FULL,self_id,remote_id);
                    return false;
                }
                // Create Transaction for current interface to transmit data
                ULSBusTransaction* transaction = _tarnsactions.open(currentConnection,self_id,remote_id,buffer,ULSBUST_AOI_TRANSMIT_START);
                if(!transaction){
                    currentConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_BUFFER_FULL,self_id,remote_id);
                    return false;
                }
            }
            if(rez == ULSBUS_OBJECT_FIND_OBJECT_NOTFOUND){
                currentConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_NOTFOUND,self_id,remote_id);
                return true;
            }
            if(rez == ULSBUS_OBJECT_FIND_OBJECT_SIZE_MISMUCH){
                currentConnection->sendAck(ULSBUS_ACK_RROI_OBJECT_SIZE_MISMUCH,self_id,remote_id);
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
            _ulsbus_obj_find_rezult rez = _library->find(self_id,0,obj_id,obj_size); //remote_id = 0 -> from any devices

            if(rez == ULSBUS_OBJECT_FIND_OK){
                // Create Transaction Buffer
                // Fill buffer with data.
                ULSBusObjectBase *obj =  _library->getObject(self_id,0,obj_id); // remote_id = 0 -> from any devices
                // check if we found object
                if(!obj)return false;
                obj->setData(obj_data);
                return true;
            }
            if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){
                // Redirect to other devices;
                ULSBusConnection* connection =  _connections.findId(remote_id);
                if(!connection) return true; // connection with device not found
                ULSBusTransaction* transaction = _tarnsactions.find(connection,self_id,remote_id,ULSBUST_AOI_RECEIVE_F);
                if(!transaction)return false; // To transaction to redirect
                ULSBusObjectBuffer* buffer = transaction->buffer();
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

            _ulsbus_obj_find_rezult rez = _library->find(remote_id,0,obj_id,obj_size); //m self_id = 0 -> from any devices

            if(rez == ULSBUS_OBJECT_FIND_OK){
                // Create Transaction Buffer
                ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
                if(!buffer){
                    //NO ACK for reading command
                    return false;
                }
                // Create Transaction for current interface
                ULSBusTransaction* transaction = _tarnsactions.open(currentConnection,self_id,remote_id,buffer,ULSBUST_AOI_RECEIVE_START);
                if(!transaction){
                    //NO ACK for reading command
                    return false;
                }
                return true;
            }
            if(rez == ULSBUS_OBJECT_FIND_DEVICE_NOTFOUND){
                // Redirect to other devices;
                ULSBusConnection* connection =  _connections.findId(remote_id);
                if(!connection) return false; // connection with device not found
                // Redirect request to next device;
                connection->send(&(currentConnection->interface()->rxBufInstance));
                // Create Transaction Buffer
                ULSBusObjectBuffer* buffer =_oBuf.open(obj_id,obj_size);
                if(!buffer){
                    //NO ACK for reading command
                    return false;
                }
                // Create Transaction for current interface to receive data
                ULSBusTransaction* transaction = _tarnsactions.open(currentConnection,self_id,remote_id,buffer,ULSBUST_AOI_RECEIVE_START);
                if(!transaction){
                    //NO ACK for reading command
                    return false;
                }
                // create transction to transmit data
                transaction = _tarnsactions.open(connection,self_id,remote_id,buffer,ULSBUST_AOI_TRANSMIT_START);
                if(!transaction){
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
            ULSBusTransaction* transaction = _tarnsactions.find(currentConnection,pxPack->aoi_f.self_id,pxPack->aoi_f.remote_id,ULSBUST_AOI_RECEIVE_F);
            if(transaction) transaction->processPacket();
        }
            break;
            //case    ULSBUS_AOI_C:    //Answer OI complete (AIO-C)
        }
        return false;
    }
private:
    ULSBusTransactionsList<64> _tarnsactions;
    ULSBusConnectionsList<4> _connections;

    ULSBusObjectsLibrary    *_library;
    ULSBusObjectBufferList<10> _oBuf;

    uint32_t _nmTimeout;

};



#endif // ULSBUS_H
