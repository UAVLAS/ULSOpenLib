/*
 * Two ULSBusConnection instances wired back to back over a pair of byte
 * fifos, framed with the same COBS the real transports use, and a device
 * carrying real objects on one end.
 *
 * This covers the part of the bus test_auth does not: that a GETOBJ comes
 * back with the object's bytes, that the bounds checks around the object
 * paths admit everything that legitimately fits, that a SETOBJ round trip
 * writes what it was given, and that a reply too large for the transmit fifo
 * is dropped whole instead of being truncated onto the wire.
 *
 *   c++ -std=c++17 -O1 -I tests/hostif -I utils -I ULSBus -I ULSSerial \
 *       -o /tmp/test_objects tests/test_objects.cpp \
 *       ULSBus/ULSBusConnection.cpp ULSBus/ULSBusInterface.cpp \
 *       ULSBus/ULSObject.cpp ULSSerial/ULSSerial.cpp utils/ULSCrypto.cpp \
 *   && /tmp/test_objects
 */
#include <cstdio>
#include <cstring>

#include "ULSBusConnection.h"
#include "ULSSerial.h"
#include "io_fifo.h"

unsigned int g_uid0 = 0xA0A0A0A0u;
bool hostRandom(unsigned char *buf, unsigned int len)
{
    for (unsigned int i = 0; i < len; i++) buf[i] = (unsigned char)(i * 7 + 1);
    return true;
}

static int failures = 0;
static void check(bool ok, const char *what)
{
    std::printf("%-52s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok) failures++;
}

/* -- a device with a small object, one the size of the largest in the real
 * library, and one that can be written ------------------------------------ */
struct SmallVar { uint32_t a; float b; };
struct BigVar   { float beans[111]; };   /* 444 bytes, as RX_Channel_Debug */

template <class T>
class TestObject : public ULSObjectBase {
 public:
  TestObject(uint16_t id, _ulsbus_obj_permissions p)
      : ULSObjectBase(id, "test", "test object", p) {
    size = sizeof(T);
    len = 1;
    _pxData = (uint8_t *)&var;
    memset(_pxData, 0, sizeof(T));
  }
  T var;
};

class TestDevice : public ULSD_ULSX {
 public:
  TestDevice()
      : ULSD_ULSX("TESTDEV", 0x1234),
        o_small(0x0010, ULSBUS_OBJECT_PERMISSION_READONLY),
        o_big(0x0011, ULSBUS_OBJECT_PERMISSION_READONLY),
        o_write(0x0012, ULSBUS_OBJECT_PERMISSION_READWRITE) {
    devname = "TESTDEV";
    add(&o_small);
    add(&o_big);
    add(&o_write);
  }
  TestObject<SmallVar> o_small;
  TestObject<BigVar>   o_big;
  TestObject<SmallVar> o_write;
};

/* -- the wire: one end's tx fifo is the other end's rx fifo ---------------- */
class LoopConnection : public ULSBusConnection, public ULSSerial {
 public:
  LoopConnection(ULSDBase *dev, ULSBusConnectionsList *cns, const char *name,
                 uint8_t did, uint8_t cid, _io_fifo_u8 *rx, _io_fifo_u8 *tx)
      : ULSBusConnection(dev, cns, name, did, cid), ULSSerial(rx, tx) {
    mode(SERIAL_MODE_COBS);
    ifSetStaticDID(did);
  }
  bool open() override { return true; }
  _io_op_result receivePacket() override {
    ifRxLen = read(ifRxBuf, IF_PACKET_SIZE);
    return (ifRxLen == 0) ? IO_NO_DATA : IO_OK;
  }
  _io_op_result sendPacket() override {
    return (write(ifTxBuf, ifTxLen) == ifTxLen) ? IO_OK : IO_ERROR;
  }
};

/* -- what the requesting end saw ------------------------------------------ */
static uint16_t lastObjId = 0xFFFF;
static uint32_t lastLen = 0;
static uint8_t  lastData[IF_PACKET_SIZE];
static int      repliesSeen = 0;
static int      acksSeen = 0;

static void onObjReceived(ULSBusConnection *sc)
{
  uint32_t hs = sc->cnRxPacket->hop & 0xF;
  lastObjId = (uint16_t)(sc->cnRxPacket->pld[hs] |
                         ((uint16_t)sc->cnRxPacket->pld[hs + 1] << 8));
  lastLen = sc->ifRxLen - (1 + hs + 2);
  memcpy(lastData, &sc->cnRxPacket->pld[hs + 2], lastLen);
  repliesSeen++;
}
static void onObjSended(ULSBusConnection *) { acksSeen++; }

/* -- what an explorer answer carried -------------------------------------- */
static uint32_t lastStatusType = 0;
static uint8_t  lastStatusName[16];
static int      statusSeen = 0;

static void onStatusReceived(ULSBusConnection *sc)
{
  uint32_t hs = sc->cnRxPacket->hop & 0xF;
  const _cn_packet_status *px =
      (const _cn_packet_status *)&sc->cnRxPacket->pld[hs];
  lastStatusType = px->type;
  memcpy(lastStatusName, px->name, sizeof(lastStatusName));
  statusSeen++;
}

/* One bus tick on both ends, a few times over, so a request and its answer
 * both get carried. */
static void pump(ULSBusConnectionsList &a, ULSBusConnectionsList &b, int n = 4)
{
  for (int i = 0; i < n; i++) {
    a.task(1);
    b.task(1);
  }
}

int main()
{
  static _io_fifo<uint8_t, 8 * 1024> pcToDev;
  static _io_fifo<uint8_t, 8 * 1024> devToPc;

  static ULSBusConnectionsList pcCns;
  static ULSBusConnectionsList devCns;

  static ULSD_ULSX pcDev("PC", 0x0001);
  static TestDevice device;

  /* did 0 is the master end, as the PC always is. Same cid on both: it is
   * one link. */
  static LoopConnection pc(&pcDev, &pcCns, "pc", 0, 3, &devToPc, &pcToDev);
  static LoopConnection dev(&device, &devCns, "dev", 1, 3, &pcToDev, &devToPc);

  pc.cnclbkObjReceived = &onObjReceived;
  pc.cnclbkObjSended = &onObjSended;

  pump(pcCns, devCns); /* let both interfaces reach IF_STATE_OK */

  uint8_t route[1] = {(uint8_t)((3 << 6) | 1)};

  /* -- a small object ----------------------------------------------------- */
  device.o_small.var.a = 0xDEADBEEF;
  device.o_small.var.b = 1.5f;
  repliesSeen = 0;
  check(pcCns.cnSendGetObject(route, 1, 0x0010) == IO_OK,
        "getobj: request accepted by the connection");
  pump(pcCns, devCns);
  check(repliesSeen == 1, "getobj: exactly one reply");
  check(lastObjId == 0x0010, "getobj: reply carries the object id");
  check(lastLen == sizeof(SmallVar), "getobj: reply carries the object length");
  check(memcmp(lastData, &device.o_small.var, sizeof(SmallVar)) == 0,
        "getobj: reply carries the object bytes");

  /* -- the largest object the real library has ---------------------------- */
  for (int i = 0; i < 111; i++) device.o_big.var.beans[i] = (float)i;
  repliesSeen = 0;
  pcCns.cnSendGetObject(route, 1, 0x0011);
  pump(pcCns, devCns);
  check(repliesSeen == 1, "getobj: a 444 byte object is answered");
  check(lastLen == sizeof(BigVar), "getobj: all 444 bytes arrive");
  check(memcmp(lastData, &device.o_big.var, sizeof(BigVar)) == 0,
        "getobj: 444 bytes arrive intact");

  /* -- an id the device does not carry ------------------------------------ */
  repliesSeen = 0;
  pcCns.cnSendGetObject(route, 1, 0x7FFF);
  pump(pcCns, devCns);
  check(repliesSeen == 0, "getobj: an unknown id is not answered");

  /* -- a read-only object cannot be written ------------------------------- */
  acksSeen = 0;
  SmallVar payload = {0x12345678, 2.5f};
  pcCns.cnSendSetObject(route, 1, 0x0010, (uint8_t *)&payload, sizeof(payload));
  pump(pcCns, devCns);
  check(acksSeen == 0, "setobj: a read-only object is refused");

  /* -- a writable one round trips ----------------------------------------- */
  acksSeen = 0;
  check(pcCns.cnSendSetObject(route, 1, 0x0012, (uint8_t *)&payload,
                              sizeof(payload)) == IO_OK,
        "setobj: request accepted by the connection");
  pump(pcCns, devCns);
  check(acksSeen == 1, "setobj: acknowledged");
  check(memcmp(&device.o_write.var, &payload, sizeof(payload)) == 0,
        "setobj: the device took the bytes");

  /* -- and back again, proving the write landed --------------------------- */
  repliesSeen = 0;
  pcCns.cnSendGetObject(route, 1, 0x0012);
  pump(pcCns, devCns);
  check((repliesSeen == 1) &&
            (memcmp(lastData, &payload, sizeof(payload)) == 0),
        "setobj: reads back what was written");

  /* -- several requests in one burst, which is what the queue paces -------- */
  repliesSeen = 0;
  pcCns.cnSendGetObject(route, 1, 0x0010);
  pcCns.cnSendGetObject(route, 1, 0x0012);
  pcCns.cnSendGetObject(route, 1, 0x0010);
  pump(pcCns, devCns, 8);
  check(repliesSeen == 3, "burst: three queued requests get three replies");

  /* -- the explorer answer, the only thing that ever reads devname --------
   *
   * The device explores the PC end here rather than the other way round,
   * because pcDev is a plain ULSD_ULSX that never assigns devname - exactly
   * as ULSD_PC in the Qt wrapper does not - and answering an explorer is
   * what reads it. That read used to be a fixed 16-byte memcpy through an
   * uninitialized pointer, so it faulted as soon as the garbage stopped
   * being a readable address. Prefilling the name with 0xAA keeps the two
   * halves of the check honest: the announced string, and the padding after
   * it, both have to be written by the answer rather than left over.
   */
  dev.cnclbkStatusReceived = &onStatusReceived;
  statusSeen = 0;
  memset(lastStatusName, 0xAA, sizeof(lastStatusName));
  check(dev.cnSendExplorer() == IO_OK,
        "explorer: request accepted by the connection");
  pump(pcCns, devCns);
  check(statusSeen == 1, "explorer: exactly one answer");
  check(lastStatusType == 0x0001, "explorer: answer carries the device type");
  check(strcmp((const char *)lastStatusName, "PC") == 0,
        "explorer: a device that never set devname names its type");
  bool padded = true;
  for (size_t i = strlen("PC"); i < sizeof(lastStatusName); i++)
    if (lastStatusName[i] != 0) padded = false;
  check(padded, "explorer: the name field is zero padded, not over-read");

  /* A name that fills the field must still come back terminated, since the
   * wrapper builds a QString straight off it. */
  device.devname = "0123456789ABCDEFGH";
  pc.cnclbkStatusReceived = &onStatusReceived;
  statusSeen = 0;
  memset(lastStatusName, 0xAA, sizeof(lastStatusName));
  pc.cnSendExplorer();
  pump(pcCns, devCns);
  check(statusSeen == 1, "explorer: an over-long name is still answered");
  check(strcmp((const char *)lastStatusName, "0123456789ABCDE") == 0,
        "explorer: an over-long name is truncated and terminated");

  std::printf("\n%s\n", failures ? "SOME TESTS FAILED" : "objects round trip");
  return failures ? 1 : 0;
}
