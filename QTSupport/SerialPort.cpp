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
    //    connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)), this,
    //            SLOT(handleError(QSerialPort::SerialPortError)));

    serialPort->waitForReadyRead(1);
}
bool SerialPort::open()
{
    serialPort->setPortName(portName);
    serialPort->setBaudRate(portBaudrate);
    return serialPort->open(QSerialPort::ReadWrite);
};
void SerialPort::closePort()
{
    _opened = false;
   // if(!opened())return;
    serialPort->close();
    qDebug()  << "Port closed.";
}
bool SerialPort::opened()
{
    return  _opened && serialPort->isOpen();//serialPort->isOpen();
}
bool SerialPort::openPort(const QString &name)
{

    QString port = findPort(name);

    if(port.length()){
       // close();
        #ifdef Q_OS_LINUX
            portName = "/dev/" + port;
        #else
            portName = port;
        #endif
        portBaudrate = 115200;
        if (open()) {
            _opened = true;
            return true;
        }
    }
    return false;
}

QString SerialPort::findPort(const QString &name)
{
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &serialPortInfo : serialPortInfos) {
        if(serialPortInfo.portName().contains(name) || serialPortInfo.manufacturer().contains(name) ){
            qDebug()  << "Found port: "<<serialPortInfo.portName()<<" Manufacturer:" << serialPortInfo.manufacturer();
            return serialPortInfo.portName();
        }
    }
    return "";
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
QStringList SerialPort::getPortsManufacturersList()
{
    QStringList portsList;
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &serialPortInfo : serialPortInfos) {
        portsList.append(serialPortInfo.manufacturer());
    }
    return portsList;
}

void SerialPort::handleReadyRead()
{    if(!opened())return;
    QByteArray data = serialPort->readAll();
    for(int i = 0; i < data.length();i++)
    {
        _rxFifo.push(data.at(i));
    }
}
void SerialPort::transmitterUpdate(){
    uint8_t ch;
    if(!opened()){_txFifo.flush();return;}
    while(_txFifo.pull(&ch)){
        if(!serialPort->putChar(ch)){
           qDebug()  << "Port closed. Putchar";
            closePort();
            return;
        }
    }
}

void SerialPort::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::NoError)return;

//    qDebug()  << QObject::tr("An I/O error occurred "
//                         "the data from port %1, error: %2")
//                 .arg(serialPort->portName())
//                 .arg(serialPort->errorString())
//              << "";
    if ((serialPortError == QSerialPort::ResourceError) ||
        (serialPortError == QSerialPort::PermissionError))
    {
        qDebug()  << QObject::tr("An I/O error occurred while reading "
                             "the data from port %1, error: %2")
                     .arg(serialPort->portName())
                     .arg(serialPort->errorString())
                  << "";
        //qDebug()  << "Port closed. Form error";
        closePort();
    }
}

ULSSerialPort::ULSSerialPort(QObject *parent,ULSDBase *dev,ULSBusConnectionsList* connections):
    ULSBusConnection(dev,connections,"PC",5,3),
    SerialPort(parent)
{

}
_io_op_rezult ULSSerialPort::receivePacket()
{
    ifRxLen = ULSSerial::read(ifRxBuf,IF_PACKET_SIZE);
    if(ifRxLen == 0)return IO_NO_DATA;
    return IO_OK;
};
_io_op_rezult ULSSerialPort::sendPacket()
{
    uint32_t len = ULSSerial::write(ifTxBuf,ifTxLen);
    ifTxLen = 0;
    if(len != ifTxLen)return IO_ERROR;
    return IO_OK;
};
