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

#include "ULSBusQTWrapper.h"


ULSBusQTWrapper *ubqtw = nullptr;

static void cnklbkStatusReceived(ULSBusConnection *sc)
{
    if(ubqtw)ubqtw->cnStatusReceived(sc);
}
static void cnklbkConnected(ULSBusConnection *sc)
{
    if(sc)sc->cnSendExplorer();
}
static void cnklbkObjReceived(ULSBusConnection *sc)
{
    if(ubqtw)ubqtw->cnObjectReceived(sc);
}
static void cnklbkObjSended(ULSBusConnection *sc)
{
    if(ubqtw)ubqtw->cnObjectSended(sc);
}
ULSBusQTWrapper::ULSBusQTWrapper():
    m_dtms(50),
    m_serial(this,&m_pcDevice,&m_connections)
{
    ubqtw = this;
    m_serial.mode(SERIAL_MODE_COBS);
    m_serial.cnclbkStatusReceived = &cnklbkStatusReceived;
    m_serial.cnclbkConnected = &cnklbkConnected;
    m_serial.cnclbkObjReceived = &cnklbkObjReceived;
    m_serial.cnclbkObjSended = &cnklbkObjSended;

    QTimer *timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(m_dtms);

    m_elapsed = new QElapsedTimer();
    m_elapsed->start();
}
void ULSBusQTWrapper::onTimer()
{
    m_counter++;
    udebugTickHandler();
    udebugElspsed(m_elapsed->nsecsElapsed()/1000000);

    if(m_serial.opened()){

        m_connections.task(m_dtms);
    }else{
        if(m_serial.openPort("UAVLAS")){
            uDebug("Port openned");
            m_serial.cnSendExplorer();
        }
    }

    if((m_counter % (1000/m_dtms)) == 0) m_serial.cnSendExplorer();

    for( auto it = m_dev.begin(); it != m_dev.end(); ++it ){
        if(it.value().timeout < m_dtms){
            emit deviceDisconnected(it.key());
            m_dev.erase(it);
            break;
        }else{
            it.value().timeout -= m_dtms;
        }
    }
}
void ULSBusQTWrapper::updateDevice(const QString &route)
{
    m_dev[route].timeout = ULSQTW_DEVICE_TIMEOUT;
}
QString ULSBusQTWrapper::getRoute(ULSBusConnection *sc)
{
    uint32_t rxHs = sc->cnRxPacket->hop & 0xF;
    QString route;
    for(uint32_t i = 0 ; i < rxHs ; i++){
        route += QString(" %1:%2").arg(sc->cnRxPacket->pld[rxHs-1-i]>>6,-1,16).arg(sc->cnRxPacket->pld[rxHs-1-i]&0x3f,-2,16);
    }
    return route;
}
// CALLBACKS
void ULSBusQTWrapper::cnObjectReceived(ULSBusConnection *sc)
{
    uint32_t rxHs = sc->cnRxPacket->hop & 0xF;
    uint8_t *buf = &sc->cnRxPacket->pld[rxHs+2];
    uint16_t obj_id = *((uint16_t*)&sc->cnRxPacket->pld[rxHs]);

    QString route(getRoute(sc));

    if(m_dev.contains(route)){
        QString objName;
        QVariantMap objData  =  m_dev[route].instance->getVar(&objName,obj_id,buf);
        emit objectReceived(route,objName,objData);
    }

}
void ULSBusQTWrapper::cnStatusReceived(ULSBusConnection *sc)
{
    // uint32_t rxH = sc->cnRxPacket->hop >> 4;
    uint32_t rxHs = sc->cnRxPacket->hop & 0xF;
    _cn_packet_status *status = (_cn_packet_status*)(&sc->cnRxPacket->pld[rxHs]);

    QString name((const char*)status->name);

    QString route(getRoute(sc));

    if(!m_dev.contains(route)){
        if(!m_devsLibrary.devTypes.contains(status->type&0x7FFF)) return ; // Unkniwn device
        m_dev[route].instance = m_devsLibrary.devTypes[(status->type)&0x7FFF];
        emit deviceConnected(route,m_dev[route].instance->typeName,name);
    }
    updateDevice(route);
}
void ULSBusQTWrapper::cnObjectSended(ULSBusConnection *sc)
{
    QString route(getRoute(sc));
    if(!m_dev.contains(route))return;
    uint32_t rxHs = sc->cnRxPacket->hop & 0xF;
    uint16_t obj_id = *((uint16_t*)&sc->cnRxPacket->pld[rxHs]);

    ULSObjectBase *obj = m_dev[route].instance->getObject(obj_id);
    if(obj) emit objectSended(route,obj->name());
}
//QML CONNECTIONS
void ULSBusQTWrapper::sendObject(const QString &route,const QString &objName,const QVariantMap &value)
{
    if(!m_dev.contains(route)) return;

    QStringList list = route.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    if(list.count() == 0) return;
    uint8_t r[15];


    for (int i = 0; i < list.length(); i++)
    {
        QStringList clist = list[i].split(":");
        bool bStatus = false;
        r[i] = (clist[0].toUInt(&bStatus,16) << 6) | clist[1].toUInt(&bStatus,16);

    }
    ULSObjectBase *obj = m_dev[route].instance->getObject(objName);
    uint32_t size = obj->set(value);
    m_connections.cnSendSetObject(r,list.length(),obj->id,obj->_pxData,size);

}
void ULSBusQTWrapper::requestObject(const QString &route,const QString &objName)
{
    if(!m_dev.contains(route)) return;

    QStringList list = route.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    if(list.count() == 0) return;
    uint8_t r[15];
    for (int i = 0; i < list.length(); i++)
    {
        QStringList clist = list[i].split(":");
        bool bStatus = false;
        r[i] = (clist[0].toUInt(&bStatus,16) << 6) | clist[1].toUInt(&bStatus,16);

    }
    u_int16_t objId = m_dev[route].instance->getObjId(objName);
    m_connections.cnSendGetObject(r,list.length(),objId);
}

void ULSBusQTWrapper::exploreDevices()
{
    m_dev.clear();
    m_connections.cnSendExplorer();
}
