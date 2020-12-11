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

#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QtCore>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QString>
#include <QList>

#include "ULSBusConnection.h"
#include "ULSSerial.h"

class SerialPort : public QObject, public ULSBusConnection, public ULSSerial {
    Q_OBJECT

// QT Serial port
public:
    explicit SerialPort(QObject *parent = 0);

    bool open() override;
    void close(void) override;

    _if_op_rezult sendBuffer() override;
    _if_op_rezult receiveBuffer() override;

    bool opened();
    QStringList getPortsList(void);
    bool openPort(QString name);

protected:
    void transmitterUpdate() override;
private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);
public:
   QString      portName;
   long         portBaudrate;

private:
    QSerialPort* _serialPort;
    _io_fifo<uint8_t,8*1024> _rxFifo;
    _io_fifo<uint8_t,8*1024> _txFifo;
};

#endif // SERIALPORT_H
