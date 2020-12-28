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

#ifndef ULSBUSQTWRAPPER_H
#define ULSBUSQTWRAPPER_H

#define ULS_DEBUG

#include<QTimer>
#include"SerialPort.h"
#include "ULSBusConnection.h"
#include "ULSDevice_ULSQX.h"
#include "ULSSerial.h"
#include <QTextStream>
#include <QElapsedTimer>

#define ULSQTW_DEVICE_TIMEOUT   2000


typedef struct{
    uint32_t timeout;
    ULSDBase* instance;
}_device_state;



class ULSBusQTWrapper: public QObject {
    Q_OBJECT
public:
    ULSBusQTWrapper();

    void sendObject(const QString &route,const QString &objName,const QVariantMap &objData);
    void requestObject(const QString &route,const QString &objName);
    void exploreDevices();

    void cnObjectReceived(ULSBusConnection *sc);
    void cnStatusReceived(ULSBusConnection *sc);

private:
    QString getRoute(ULSBusConnection *sc);
    void updateDevice(const QString &route);

private:
    uint m_dtms;
    uint     m_counter;
    QElapsedTimer* m_elapsed;
    ULSD_PC m_pcDevice;
    ULSBusConnectionsList m_connections;
    ULSSerialPort m_serial;
    ULSQTDevicesLibrary   m_devsLibrary;
    QHash<QString,_device_state> m_dev;

signals:
    void objectReceived(const QString &route,const QString &objName,const QVariantMap &objData);
    void deviceConnected(const QString &route,const  QString &deviceType,const  QString &deviceName);
    void deviceDisconnected(const QString &route);
private slots:
    void onTimer();

};


#endif // ULSBUSQTWRAPPER_H
