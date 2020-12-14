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

#include "SerialPort.h"

SerialPort::SerialPort(QObject *parent) :
    QObject(parent),
    ULSSerial(&_rxFifo,&_txFifo),
    _opened(false)
{
    serialPort = new QSerialPort();

    connect(serialPort, &QSerialPort::readyRead, this, &SerialPort::handleReadyRead);
    connect(serialPort, &QSerialPort::errorOccurred, this, &SerialPort::handleError);

    serialPort->waitForReadyRead(1);
}
bool SerialPort::open()
{
    serialPort->setPortName(portName);
    serialPort->setBaudRate(portBaudrate);
    return serialPort->open(QSerialPort::ReadWrite);
};
void SerialPort::close()
{
    serialPort->close();
}
bool SerialPort::opened()
{
    return  _opened;//serialPort->isOpen();
}
bool SerialPort::openPort(QString name)
{
    QStringList ports = getPortsList();
    foreach (QString p,ports) {
        if (p.contains(name)) {
            name = p;
            break;
        }
    }
    portName = name;
    portBaudrate = 115200;
    if (open()) {
        _opened = true;
    }
    return false;
}


QStringList SerialPort::getPortsList()
{
    QStringList portsList;
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &serialPortInfo : serialPortInfos) {
        portsList.append(serialPortInfo.portName());
    }
    return portsList;
}

void SerialPort::handleReadyRead()
{
    QByteArray data = serialPort->readAll();
    for(int i = 0; i < data.length();i++)
    {
        _rxFifo.push(data.at(i));
    }
}
void SerialPort::transmitterUpdate(){
    uint8_t ch;
    while(_txFifo.pull(&ch))serialPort->putChar(ch);
}

void SerialPort::handleError(QSerialPort::SerialPortError serialPortError)
{
     if (serialPortError == QSerialPort::ResourceError)
     {
    if (serialPortError == QSerialPort::ReadError) {
        qDebug()  << QObject::tr("An I/O error occurred while reading "
                                 "the data from port %1, error: %2")
                     .arg(serialPort->portName())
                     .arg(serialPort->errorString())
                  << Qt::endl;
     //   QCoreApplication::exit(1);
    }
    _opened = false;
    serialPort->close();
     }
}

ULSSerialPort::ULSSerialPort(QObject *parent):
    ULSBusConnection("PC",255,0),
    SerialPort(parent)
{

}
_io_op_rezult ULSSerialPort::receivePacket()
{
    if(! serialPort->isOpen())return IO_ERROR;
    ifRxLen = ULSSerial::read(ifRxBuf,IF_PACKET_SIZE);
    if(ifRxLen == 0)return IO_NO_DATA;
    return IO_OK;
};
_io_op_rezult ULSSerialPort::sendPacket()
{
    if(! serialPort->isOpen())return IO_ERROR;
    uint32_t len = ULSSerial::write(ifTxBuf,ifTxLen);
    ifTxLen = 0;
    if(len != ifTxLen)return IO_ERROR;
    return IO_OK;
};
