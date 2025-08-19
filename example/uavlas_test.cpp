#include <iostream>
#include "../ULSBus/ULSObject.h"
#include "../ULSBus/ULSBlitzTypes.h"
#include "../ULSBus/ULSBusInterface.h"
#include "../ULSBus/ULSBusConnection.h"
#include "../ULSSerial/ULSSerial.h"
#include "../ULSBus/ULSBusTypes.h"
#include "../ULSBus/ULSQTObject.hpp"
#include "../utils/io_fifo.h"

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <chrono>

/*
g++ -g uavlas_test.cpp -I../ULSBus/ -I../utils -I../ULSSerial ../ULSBus/ULSBusConnection.cpp ../ULSBus/ULSBusInterface.cpp ../ULSBus/ULSObject.cpp ../ULSSerial/ULSSerial.cpp ../utils/udebug.c -o uavlas_test
*/


// Define packed structures for GCC (Linux)
#define PACKED_STRUCT struct __attribute__((packed))
#define PACKED_END

// Define the status structure based on ULSDevices.md for ULSDVOBJ_RX_Status_v1
PACKED_STRUCT ULSQR1R2Status {
    uint32_t status;                // Device status bitfield
    uint32_t error;                 //Device error bitfield
    uint32_t blitzTest;             //Debug
    uint16_t packCntr;              //Debug
    uint16_t uartErrors;            //Uart errors counter
    uint16_t uartTxPacks;           //Uart packets transmitted
    uint16_t uartRxPacks;           //Uart packets received
    uint16_t i2cErrors;             //I2C errors counter
    uint16_t i2cTxPacks;            //I2C packets transmitted
    uint16_t i2cRxPacks;            //I2C packets received
    uint16_t bitMax;                //Debug
    uint8_t qtId;                   //Debug
    uint8_t synqChannel;            //Debug
    uint32_t posTime;               // Debug
    uint32_t emsTime;               //Debug
    uint32_t synqMax;               //Debug
    float level;                    // Signal level
    float levelA;                   //Signal level A channel
    float levelB;                   //Signal level B channel
    float snrA;                     //Signal to noise ratio for channel A
    float snrB;                     //Signal to noise ratio for channel B
    float prob;                     // Signal probability
    float dacsDistance;             //Distance provided by MRX algorithm
    float mrxDistance;              //Distance provided by MRX algorithm
    float distance;                 //Distance to transmitter
    float mrxYaw;                   //Yaw orintation provided by MRX
    float dacsYaw;                  //Yaw orintation provided by DACS
    float correctedYaw;             //Yaw orintation provided by calculated
    float dacs_power[4];            //DACS Illumination power
    float ang[3];                   //Receiver angular position in transmitter frame
    float pos[3];                   //Receiver position in transmitter frame
    float vel[3];                   //Receiver velocity in transmitter frame
    float gimu[3];                  //Transmitter orientation(roll, pitch, yaw)
    float rel_pos_ned[3];           //Relative Position of transmitter(offset from receiver)in NED(North East Down)
    float rel_vel_ned[3];           //Ralative velocity of tranmitter(offset from receiver) in NED(North East Down)
    float rel_pos_frd[3];           //Relative Position of transmitter(offset from receiver) in FRD(Forward Right Down)
    float rel_vel_frd[3];           //Ralative velocity of tranmitter(offset from receiver) in FRD(Forward Right Down)
    float pos_wld[3];               //World Position of transmitter(Lat Lon Msl)
    float vel_wld[3];               //World velocity of tranmitter in NED(North East Down)
    float abs_pos_ned[3];           //Absolute position of transmitter(vehicle frame) in NED(North East Down)
    float abs_vel_ned[3];           // Absolute velocity of tranmitter(vehicle frame)in NED(North East Down)
    float vehicle_abs_pos_ned[3];   //Absolute Position of vehicle in NED(North East Down)
    float vehicle_abs_vel_ned[3];   //Absolute velocity of vehicle in NED(North East Down)
    float vehicle_heading;          //Vehicle heading information
} PACKED_END;

// Define the debug structure for ULSQR1R2Debug
PACKED_STRUCT ULSQR1R2Debug {
    float beansA[37];               //Debug datareceiver A
    float beansB[37];               //Debug datareceiver B
} PACKED_END;

// Structure for CN_ACK_GETOBJ payload
PACKED_STRUCT CN_ACK_GETOBJ_Payload {
    uint8_t status;     // Status byte
    uint16_t obj_addr;  // Object address
    uint8_t buf[512];   // Object data
} PACKED_END;

// LinuxULSSerial class for Linux serial communication
class LinuxULSSerial : public ULSSerial {
private:
    int fdSerial = -1;
    _io_fifo<uint8_t, 4096> rxBuf;
    _io_fifo<uint8_t, 4096> txBuf;

public:
    LinuxULSSerial() : ULSSerial(&rxBuf, &txBuf) {}

    ~LinuxULSSerial() { close(); }

    bool open(const std::string& port, speed_t baudrate) {
        fdSerial = ::open(port.c_str(), O_RDWR | O_NOCTTY );
        if (fdSerial == -1) {
            std::cerr << "Error opening serial port: " << strerror(errno) << std::endl;
            return false;
        }

        // Make the file descriptor blocking (just to be explicit)
        int flags = fcntl(fdSerial, F_GETFL, 0);
        if (flags != -1) fcntl(fdSerial, F_SETFL, flags & ~O_NONBLOCK);

        struct termios tio{};
        if (tcgetattr(fdSerial, &tio) != 0) {
            std::cerr << "tcgetattr error: " << strerror(errno) << std::endl;
            close();
            return false;
        }

        cfmakeraw(&tio);                 // raw input/output (clears iflag, oflag, cflag, lflag bits appropriately)
        cfsetispeed(&tio, baudrate);
        cfsetospeed(&tio, baudrate);

        // 8N1, receiver enabled, ignore modem control
        tio.c_cflag |= (CLOCAL | CREAD);
        tio.c_cflag &= ~PARENB;
        tio.c_cflag &= ~CSTOPB;
        tio.c_cflag &= ~CSIZE;
        tio.c_cflag |= CS8;

        // No software flow control
        tio.c_iflag &= ~(IXON | IXOFF | IXANY);

        // "Overlapped-like" behavior: do not block forever, but wait a short time for bytes.
        // VMIN=0, VTIME=1 -> read() returns immediately if data is available; otherwise waits up to 100ms.
        tio.c_cc[VMIN]  = 0;
        tio.c_cc[VTIME] = 0;   // 0.1s

        if (tcsetattr(fdSerial, TCSANOW, &tio) != 0) {
            std::cerr << "Error setting serial state: " << strerror(errno) << std::endl;
            close();
            return false;
        }

        // Clear any stale bytes
        tcflush(fdSerial, TCIOFLUSH);

        _isOpened = true;
        return true;
    }

    void close() {
        if (fdSerial != -1) {
            ::close(fdSerial);
            fdSerial = -1;
        }
        _isOpened = false;
    }

    void sendRaw(const uint8_t* raw, uint32_t len) {
        _serial_mode prevMode = mode();
        mode(SERIAL_MODE_RAW);
        write(const_cast<uint8_t*>(raw), len);
        transmitterUpdate();
        mode(prevMode);
    }

    void receiverUpdate() override  {
        if (!_isOpened) return;

        // Read in a tight loop to drain the driver buffer completely each time we’re called.
        uint8_t temp[256];
        for (;;) {
            ssize_t n = ::read(fdSerial, temp, sizeof(temp));
            if (n > 0) {
                _rxFifo->push(temp, static_cast<uint32_t>(n));
                // Continue reading until the kernel says there's nothing more right now
                continue;
            }
            if (n == 0) {
                // VTIME timeout expired with no data; nothing more to read right now.
                break;
            }
            // n < 0
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Nothing available at the exact moment.
                break;
            } else if (errno == EINTR) {
                // Interrupted system call, retry.
                continue;
            } else {
                std::cerr << "Read error: " << strerror(errno) << std::endl;
                break;
            }
        }
    }

    void transmitterUpdate() override {
        if (!_isOpened) return;

        for (;;) {
            uint32_t avail = _txFifo->count_to_edge();
            if (avail == 0) break;

            uint8_t* p = _txFifo->head();
            ssize_t wrote = ::write(fdSerial, p, avail);
            if (wrote > 0) {
                _txFifo->flush(static_cast<uint32_t>(wrote));
                // loop to flush more if available
                continue;
            }
            if (wrote < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                // Driver not ready right now; try again on the next pump
                break;
            }
            if (wrote < 0) {
                std::cerr << "Write error: " << strerror(errno) << std::endl;
                break;
            }
        }

        // Consider TX "complete" if FIFO is empty
        _txCplt = _txFifo->empty();
    }
};

// SerialBusConnection class (modified for Linux)
class SerialBusConnection : public ULSBusConnection {
private:
    LinuxULSSerial serial;
    uint8_t deviceID = 255;

public:
    SerialBusConnection(ULSDBase* dev, ULSBusConnectionsList* connections, const char* name, uint8_t did, uint8_t cid)
        : ULSBusConnection(dev, connections, name, did, cid) {}

    bool open() override {
        //if (serial.open("/dev/ttyACM2", B115200)) { // /dev/ttyACM0 /dev/serial/by-id/usb-UAVLAS_Virtual_ComPort_UAVLAS-if00
        if (serial.open("/dev/serial/by-id/usb-UAVLAS_Virtual_ComPort_UAVLAS-if00", B115200)) {
            serial.mode(SERIAL_MODE_COBS);
            
            return true;
        }
        return false;
    }

    void close() override {
        serial.close();
    }

    _io_op_result sendPacket() override {
        serial.write(ifTxBuf, ifTxLen);
        serial.transmitterUpdate();
        return IO_OK;
    }

    _io_op_result receivePacket() override {
        serial.receiverUpdate();
        uint32_t len = serial.read(ifRxBuf, IF_PACKET_SIZE);
        if (len > 0) {
            ifRxLen = len;
            return IO_OK;
        }
        return IO_NO_DATA;
    }

    void deviceConnected(uint8_t id) override {
        std::cout << "Device connected with ID: 0x" << std::hex << static_cast<int>(id) << std::endl;
        deviceID = id;
    }

    uint8_t getDeviceID() const { return deviceID; }

    void sendRaw(const uint8_t* raw, uint32_t len) {
        serial.sendRaw(raw, len);
    }
};

void onObjectReceived(ULSBusConnection* conn) {
    if (conn->cnRxPacket->cmd == CN_ACK_GETOBJ) {
        CN_ACK_GETOBJ_Payload* payload = reinterpret_cast<CN_ACK_GETOBJ_Payload*>(conn->cnRxPacket->pld);
        if (payload->obj_addr == 0x0010) {  // Check for status object
            ULSQR1R2Status status;
            memcpy(&status, payload->buf, sizeof(ULSQR1R2Status));
            std::cout << "Status: " << std::dec << std::fixed << std::setprecision(3) << status.packCntr << ", "
                      << status.distance << ", " << status.dacsYaw << ", "
                      << status.pos[0] << ", " << status.pos[1] << ", " << status.pos[2] << ", " 
                      << status.rel_pos_ned[0] << ", " << status.rel_pos_ned[1] << ", " << status.rel_pos_ned[2] << ", " 
                      << status.abs_pos_ned[0] << ", " << status.abs_pos_ned[1] << ", " << status.abs_pos_ned[2] << ", " 
                      << status.vehicle_abs_pos_ned[0] << ", " << status.vehicle_abs_pos_ned[1] << ", " << status.vehicle_abs_pos_ned[2] << ", " 
                      << status.pos_wld[0] << ", " << status.pos_wld[1] << ", " << status.pos_wld[2]
                      << std::endl;
        } else if (payload->obj_addr == 0x0030) { // Check for debug object
            ULSQR1R2Debug debug;
            memcpy(&debug, payload->buf, sizeof(ULSQR1R2Debug));
            std::cout << "DBG: " << std::dec << std::fixed << std::setprecision(3)
                      << debug.beansA[0] << ", " << debug.beansA[1] << ", " << debug.beansA[2] << ", "
                      << debug.beansA[3] << ", " << debug.beansA[4] << ", " << debug.beansA[5] << std::endl;
        }
    }
}

int main() {
    using namespace std::chrono;

    ULSBusConnectionsList connections;
    SerialBusConnection conn(nullptr, &connections, "SerialConn", IF_LOCAL_DEVICES_NUM, 0);
    conn.cnclbkObjReceived = onObjectReceived;

    if (!conn.open()) {
        std::cerr << "Failed to open connection" << std::endl;
        return 1;
    }
    
    conn.cnSendExplorer();

    auto lastRequestTime = steady_clock::now();

    while (true) {
        conn.task(1);  // Process with 1 ms delta time
        auto now = steady_clock::now();
        if (conn.getDeviceID() != 255 && duration_cast<milliseconds>(now - lastRequestTime).count() >= 50) {
            uint8_t route[] = { conn.getDeviceID() };
            conn.cnSendGetObject(route, 1, 0x0010); // Status
            //conn.cnSendGetObject(route, 1, 0x0030); // Debug
            lastRequestTime = now;
        }

        // tiny sleep to prevent 100% CPU load
        usleep(1000); // 1 ms
    }
    
    conn.close();
    return 0;
}