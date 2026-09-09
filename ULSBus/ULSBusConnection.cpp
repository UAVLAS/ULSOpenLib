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
 * Some open source application is distributed in the hop that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license LGPL-3.0+ <https://spdx.org/licenses/LGPL-3.0+>
 */

#include "ULSBusConnection.h"

ULSBusConnection::ULSBusConnection(ULSDBase *dev,
                                   ULSBusConnectionsList *connections,
                                   const char *name, uint8_t did, uint8_t cid)
    : ULSListItem(),
      ULSBusInterface(name, did),
      cnclbkConnected(nullptr),
      cnclbkStatusReceived(nullptr),
      cnclbkObjReceived(nullptr),
      cnclbkObjSended(nullptr),
      cnclbkObjRequested(nullptr),
      _cid(cid),
      _connections(connections) {
  _dev = dev;
  connections->add(this);
  cnRxPacket = (_cn_packet *)ifRxBuf;
  cnTxPacket = (_cn_packet *)ifTxBuf;
}

void ULSBusConnection::task(uint32_t dtms) {
  cnReceive();
  ULSBusInterface::task(dtms);
};

void ULSBusConnection::deviceConnected(uint8_t id) {
  DEBUG_MSG("%s: Device Connected 0x%.2X ", _name, id);
  (void)id;
}
void ULSBusConnection::deviceDisconnected(uint8_t id) {
  DEBUG_MSG("%s: Device Disconnected 0x%.2X ", _name, id);
  (void)id;
}

void ULSBusConnection::ifOk() { CN_CALL(cnclbkConnected); }

_io_op_result ULSBusConnection::cnReceive() {
  while (ifReceive() == IO_OK) {
    if (ifRxLen < 1) continue;
    DEBUG_MSG("%s: cnReceived from:0x%.2X cmd: 0x%.2X len: %d", _name,
              cnRxPacket->src_did, cnRxPacket->cmd, ifRxLen);
    cnProcessPacket();
    ifRxLen = 0;
  }
  return IO_OK;
}
_io_op_result ULSBusConnection::cnProcessPacket() {
  uint8_t *R = cnRxPacket->pld;
  uint32_t rxH = cnRxPacket->hop >> 4;
  uint32_t rxHs = cnRxPacket->hop & 0xF;
  uint8_t rId = R[rxH] & 0x3f;
  uint8_t forwardCID = R[rxH + 1] >> 6;

  // Fix Route with  source information
  R[rxH] = (((_cid & 0x03) << 6) | (cnRxPacket->src_did & 0x3f));
  if (cnRxPacket->cmd == CN_CMD_EXPLORER) return cnProcessExplorer();

  DEBUG_MSG("%s: cnProcessPacket lid:0x%.2X cmd: 0x%.2X len: %d hs:%d h:%d",
            _name, _did, cnTxPacket->cmd, ifTxLen, rxHs, rxH);
  DEBUG_PACKET(_name, " cnProcessPacket Route", cnRxPacket->pld, rxHs);
  DEBUG_MSG("%s: cnProcessPacket Check route id [%.2X] vs Self [%.2X]", _name,
            rId, ifid());

  if (rId != ifid()) return IO_OK;  // just not our packet - forgot about it
  if ((rxH + 1) == rxHs) return cnProcessOurPacket();  // OMG it for us !!!
  _connections->cnForwardPacket(forwardCID, this);
  return IO_OK;
}
uint8_t *ULSBusConnection::cnPrepareAnswer(uint8_t cmd) {
  uint32_t rxHs = cnRxPacket->hop & 0xF;
  cnTxPacket->cmd = cmd;
  for (uint32_t i = 0; i < rxHs; i++) {  // Flip route table for response
    cnTxPacket->pld[i] = cnRxPacket->pld[rxHs - 1 - i];
  }
  cnTxPacket->hop = (0 << 4) | rxHs;  // hop  =  0; hop size = rxhop
  return &cnTxPacket->pld[cnTxPacket->hop & 0xF];
}
_io_op_result ULSBusConnection::cnProcessExplorer() {
  if ((cnRxPacket->hop & 0xF) < 15) {       // Max hop reached no forwarding
    _connections->cnForwardExplorer(this);  // Forward Message
  }
  // Prepare answer
  _cn_packet_status *px = (_cn_packet_status *)cnPrepareAnswer(CN_ACK_EXPLORER);

  memcpy((uint8_t *)px->name, _dev->devname, 16);
  px->name[15] = 0;
  px->type = _dev->typeCode;
  uint32_t txHs = cnTxPacket->hop & 0xF;
  ifTxLen = sizeof(_cn_packet_status) + txHs + 1;
  DEBUG_MSG("%s: cnAnswer lid:0x%.2X cmd: 0x%.2X len: %d", _name, _did,
            cnTxPacket->cmd, ifTxLen);
  DEBUG_PACKET(_name, "cnAnswer Route", cnTxPacket->pld, txHs);
  return ifSend();
}

/*
 * The object id sits at pld[hop size], and pld itself is at offset 3 of
 * _cn_packet. So its address is odd for every EVEN hop size - which is to say
 * for every RELAYED packet, since a directly attached peer has hop size 1 and
 * a one-hop relay has 2.
 *
 * That is fatal on ARMv6-M. The Cortex-M0+ on ULS-XX-EIGC-G3 has no unaligned
 * access support at all, so the `*(uint16_t *)` these four sites used to do
 * compiled to LDRH/STRH on an odd address and took a HardFault the first time
 * the device was asked for an object through a relay. It never showed up
 * before because that board is the first M0+ in the tree to forward anything,
 * and a packet it merely forwards is moved with memcpy and never read as a
 * halfword.
 *
 * Byte-wise, little-endian, exactly as cnSendGetObject() and
 * cnSendSetObject() already write the same field on the other side of the
 * wire. Alignment-proof everywhere and free on the M4/M7 boards, where the
 * two byte loads fold into the same work the halfword did.
 */
static inline uint16_t cnGetObjId(const uint8_t *px) {
  return (uint16_t)(px[0] | ((uint16_t)px[1] << 8));
}
static inline void cnPutObjId(uint8_t *px, uint16_t obj_id) {
  px[0] = (uint8_t)(obj_id & 0xff);
  px[1] = (uint8_t)((obj_id >> 8) & 0xff);
}

/*
 * How many bytes an object actually occupies on the wire. getData()/setData()
 * have always copied size*len; only the length written into the packet used
 * to be a bare `size`, so an object with len > 1 would have announced a short
 * packet while memcpy ran off the end of the buffer. Every generated object
 * sets len = 1, so this has never fired - it is written the consistent way so
 * it never can.
 */
static inline uint32_t cnObjWireLen(ULSObjectBase *obj) {
  uint32_t len = (obj->len == 0) ? 1u : (uint32_t)obj->len;
  return (uint32_t)obj->size * len;
}

/*
 * The largest object this hop can carry, given a route of hs bytes.
 *
 * A GETOBJ answer is: 2 interface header + 1 hop + hs route + 2 object id +
 * the object itself, and all of that has to fit in the ifTxBuf the packet is
 * assembled in. Nothing used to check it, so an oversized object simply ran
 * past the end of the buffer and corrupted whatever the linker had put after
 * it. With IF_PACKET_SIZE at 1324 the ceiling is 1318 bytes on a direct link
 * and 1304 at the maximum route depth.
 */
static inline uint32_t cnObjMaxLen(uint32_t hs) {
  const uint32_t overhead = IF_PACKET_HEADER_SIZE + 1 + hs + 2;
  return (IF_PACKET_SIZE > overhead) ? (IF_PACKET_SIZE - overhead) : 0;
}

_io_op_result ULSBusConnection::cnProcessGetObject() {
  uint32_t rxHs = cnRxPacket->hop & 0xf;
  uint16_t obj_id = cnGetObjId(&cnRxPacket->pld[rxHs]);

  ULSObjectBase *obj = _dev->getObject(obj_id);
  if (obj == nullptr) {
    DEBUG_MSG("%s: Requested Wrong Object [0x%.4X]", _name, obj_id);
    return IO_ERROR;
  }
  if ((obj->_permission != ULSBUS_OBJECT_PERMISSION_READWRITE) &&
      (obj->_permission != ULSBUS_OBJECT_PERMISSION_READONLY))
    return IO_ERROR;

  uint32_t objLen = cnObjWireLen(obj);
  if (objLen > cnObjMaxLen(rxHs)) {
    DEBUG_MSG("%s: Object [0x%.4X] too big for packet: %d > %d", _name, obj_id,
              objLen, cnObjMaxLen(rxHs));
    return IO_ERROR;
  }

  uint8_t *px = cnPrepareAnswer(CN_ACK_GETOBJ);
  cnPutObjId(px, obj_id);
  px += 2;
  obj->getData(px);
  uint32_t txHs = cnTxPacket->hop & 0xF;
  ifTxLen = 1 + txHs + 2 + objLen;
  DEBUG_MSG("%s: cnAnswer Object lid:0x%.2X cmd: 0x%.2X len: %d", _name, _did,
            cnTxPacket->cmd, ifTxLen);
  DEBUG_PACKET(_name, "cnAnswer Route", cnTxPacket->pld, txHs);
  return ifSend();
}
_io_op_result ULSBusConnection::cnProcessSetObject() {
  uint32_t rxHs = cnRxPacket->hop & 0xf;
  uint16_t obj_id = cnGetObjId(&cnRxPacket->pld[rxHs]);
  uint8_t *obj_px = ((uint8_t *)&cnRxPacket->pld[rxHs + 2]);

  ULSObjectBase *obj = _dev->getObject(obj_id);
  if (obj == nullptr) {
    DEBUG_MSG("%s: Requested Wrong Object [0x%.4X]", _name, obj_id);
    return IO_ERROR;
  }
  if ((obj->_permission != ULSBUS_OBJECT_PERMISSION_READWRITE) &&
      (obj->_permission != ULSBUS_OBJECT_PERMISSION_WRITEONLY))
    return IO_ERROR;

  /*
   * setData() copies the object's own length out of the packet, so a SETOBJ
   * that is short - truncated on the wire, or simply malformed - used to read
   * past the end of what was actually received and write the garbage into the
   * live object. ifRxLen here is the connection-layer length: hop + route +
   * object id + data.
   */
  uint32_t objLen = cnObjWireLen(obj);
  if (ifRxLen < (1 + rxHs + 2 + objLen)) {
    DEBUG_MSG("%s: SETOBJ [0x%.4X] short packet: %d < %d", _name, obj_id,
              ifRxLen, (1 + rxHs + 2 + objLen));
    return IO_ERROR;
  }

  obj->setData(obj_px);
  uint8_t *px = cnPrepareAnswer(CN_ACK_SETOBJ);
  cnPutObjId(px, obj_id);

  uint32_t txHs = cnTxPacket->hop & 0xF;
  ifTxLen = 1 + txHs + 2;
  return ifSend();
}
_io_op_result ULSBusConnection::cnProcessSys() {
  uint8_t *px = cnPrepareAnswer(CN_ACK_SYS);

  volatile _cn_sys_oprezult rez = CN_CALL_SYS(cnclbkSys);
  *((_cn_sys_oprezult *)px) = rez;

  uint32_t txHs = cnTxPacket->hop & 0xF;
  ifTxLen = 1 + txHs + 1;
  return ifSend();
}
_io_op_result ULSBusConnection::cnProcessOurPacket() {
  switch (cnRxPacket->cmd) {
    case CN_ACK_EXPLORER:
      CN_CALL(cnclbkStatusReceived);
      break;
    case CN_CMD_SYS:
      cnProcessSys();
      break;
    case CN_ACK_SYS:
      CN_CALL_SYSACK(cnclbkSysAck);
      break;
    case CN_CMD_GETOBJ:
      cnProcessGetObject();
      break;
    case CN_ACK_GETOBJ:
      CN_CALL(cnclbkObjReceived);
      break;
    case CN_CMD_SETOBJ:
      cnProcessSetObject();
      break;
    case CN_ACK_SETOBJ:
      CN_CALL(cnclbkObjSended);
      break;
  }
  return IO_OK;
}
uint8_t *ULSBusConnection::cnPreparePacket(uint8_t *route, uint8_t hs,
                                           uint8_t cmd) {
  cnTxPacket->cmd = cmd;
  cnTxPacket->hop = (0 << 4) | hs;
  memcpy(cnTxPacket->pld, route, hs);
  ifTxLen = 1 + hs;
  return &cnTxPacket->pld[hs];
}
_io_op_result ULSBusConnection::cnSendExplorer() {
  cnTxPacket->cmd = CN_CMD_EXPLORER;
  cnTxPacket->hop = (0 << 4) | 1;  // h = 0; hs = 1;
  ifTxLen = (cnTxPacket->hop & 0xF) + 1;
  return ifSend();
}
_io_op_result ULSBusConnection::cnSendStatus() {
  return cnSendExplorer();
}
_io_op_result ULSBusConnection::cnSendSysErase(uint8_t *route, uint8_t hs,
                                               uint32_t key, uint32_t start,
                                               uint32_t len) {
  if ((route[0] >> 6) != _cid) return IO_ERROR;  // not our interface
  _cn_sys_packet *pxSys =
      (_cn_sys_packet *)cnPreparePacket(route, hs, CN_CMD_SYS);
  pxSys->syscmd = CN_SYS_CMD_ERASE;
  pxSys->erase.key = key;
  pxSys->erase.start = start;
  pxSys->erase.len = len;
  ifTxLen += +1 + 12;
  return ifSend();
}
_io_op_result ULSBusConnection::cnSendSysSetMode(uint8_t *route, uint8_t hs,
                                                 _cn_sys_mode mode) {
  if ((route[0] >> 6) != _cid) return IO_ERROR;  // not our interface
  _cn_sys_packet *pxSys =
      (_cn_sys_packet *)cnPreparePacket(route, hs, CN_CMD_SYS);
  pxSys->syscmd = CN_SYS_CMD_SETMODE;
  pxSys->setmode.mode = mode;
  ifTxLen += 1 + 1;
  return ifSend();
}
_io_op_result ULSBusConnection::cnSendSysWrite(uint8_t *route, uint8_t hs,
                                               uint32_t key, uint32_t start,
                                               uint32_t len, uint8_t *buf) {
  if ((route[0] >> 6) != _cid) return IO_ERROR;  // not our interface
  if (len > 512) return IO_ERROR;                // buff too big
  _cn_sys_packet *pxSys =
      (_cn_sys_packet *)cnPreparePacket(route, hs, CN_CMD_SYS);
  pxSys->syscmd = CN_SYS_CMD_WRITE;
  pxSys->write.key = key;
  pxSys->write.start = start;
  pxSys->write.len = len;
  memcpy(pxSys->write.buf, buf, len);
  ifTxLen += 1 + 12 + len;
  return ifSend();
}
_io_op_result ULSBusConnection::cnSendSysSetSignature(
    uint8_t *route, uint8_t hs, uint32_t key, char *fw, char *ldr,
    uint32_t ftime, uint32_t progsize, uint32_t progcrc) {
  if ((route[0] >> 6) != _cid) return IO_ERROR;  // not our interface
  _cn_sys_packet *pxSys =
      (_cn_sys_packet *)cnPreparePacket(route, hs, CN_CMD_SYS);
  pxSys->syscmd = CN_SYS_CMD_SETSIGNATURE;
  pxSys->signature.key = key;
  memcpy(pxSys->signature.fw, fw, 32);
  memcpy(pxSys->signature.ldr, ldr, 32);
  pxSys->signature.progflashingtime = ftime;
  pxSys->signature.progsize = progsize;
  pxSys->signature.progcrc = progcrc;
  ifTxLen += 1 + sizeof(pxSys->signature);
  return ifSend();
}
_io_op_result ULSBusConnection::cnSendSysSaveConfig(uint8_t *route, uint8_t hs,
                                                    uint32_t key) {
  if ((route[0] >> 6) != _cid) return IO_ERROR;  // not our interface
  _cn_sys_packet *pxSys =
      (_cn_sys_packet *)cnPreparePacket(route, hs, CN_CMD_SYS);
  pxSys->syscmd = CN_SYS_CMD_SAVECFG;
  pxSys->saveCfg.key = key;
  ifTxLen += 1 + 4;
  return ifSend();
}
_io_op_result ULSBusConnection::cnSendGetObject(uint8_t *route, uint8_t hs,
                                                uint16_t obj_addr) {
  if ((route[0] >> 6) != _cid) return IO_ERROR;  // not our interface

  cnTxPacket->cmd = CN_CMD_GETOBJ;
  cnTxPacket->hop = (0 << 4) | hs;
  memcpy(cnTxPacket->pld, route, hs);

  cnTxPacket->pld[hs] = obj_addr & 0xff;
  cnTxPacket->pld[hs + 1] = (obj_addr >> 8) & 0xff;
  ifTxLen = (1 + hs + 2);
  return ifSend();
}
_io_op_result ULSBusConnection::cnSendSetObject(uint8_t *route, uint8_t hs,
                                                uint16_t obj_addr, uint8_t *buf,
                                                uint32_t size) {
  if ((route[0] >> 6) != _cid) return IO_ERROR;  // not our interface
  if (size > cnObjMaxLen(hs)) return IO_ERROR;   // would overrun ifTxBuf

  cnTxPacket->cmd = CN_CMD_SETOBJ;
  cnTxPacket->hop = (0 << 4) | hs;
  memcpy(cnTxPacket->pld, route, hs);

  cnTxPacket->pld[hs] = obj_addr & 0xff;
  cnTxPacket->pld[hs + 1] = (obj_addr >> 8) & 0xff;
  memcpy(&cnTxPacket->pld[hs + 2], buf, size);
  ifTxLen = (1 + hs + 2 + size);
  return ifSend();
}
_io_op_result ULSBusConnection::cnForwardExplorer(ULSBusConnection *sc) {
  uint32_t rxHs = sc->cnRxPacket->hop & 0xF;

  if (rxHs == 15) return IO_ERROR;  // Max hop reached

  cnTxPacket->cmd = CN_CMD_EXPLORER;
  // Copy route
  for (uint32_t i = 0; i < rxHs; i++) {
    cnTxPacket->pld[i] = sc->cnRxPacket->pld[i];
  }
  cnTxPacket->hop = sc->cnRxPacket->hop + 0x11;  // Inc hop and hopSize
  DEBUG_MSG("%s: cnForward Explorer Added route: %.2X", _name,
            cnTxPacket->pld[rxHs]);
  ifTxLen = (cnTxPacket->hop & 0xF) + 1;
  DEBUG_MSG("%s: cnForward Explorer lid:0x%.2X cmd: 0x%.2X len: %d", _name,
            _did, cnTxPacket->cmd, ifTxLen);
  return ifSend();
}
_io_op_result ULSBusConnection::cnForwardPacket(ULSBusConnection *sc) {
  memcpy(cnTxPacket->pld, sc->cnRxPacket->pld, sc->ifRxLen - 1);
  cnTxPacket->cmd = sc->cnRxPacket->cmd;
  cnTxPacket->hop = sc->cnRxPacket->hop + 0x10;

  ifTxLen = sc->ifRxLen;
  DEBUG_MSG("%s: cnForward lid:0x%.2X cmd: 0x%.2X len: %d", _name, _did,
            cnTxPacket->cmd, ifTxLen);
  return ifSend();
}

void ULSBusConnectionsList::task(uint32_t dtms) {
  begin();
  while (next()) {
    current->task(dtms);
  }
}
void ULSBusConnectionsList::cnForwardExplorer(ULSBusConnection *sc) {
  begin();
  while (next()) {
    if (current != sc) current->cnForwardExplorer(sc);
  }
}
_io_op_result ULSBusConnectionsList::cnForwardPacket(uint8_t cid,
                                                     ULSBusConnection *sc) {
  begin();
  while (next()) {
    if ((current != sc) && (current->cid() == cid)) {
      return current->cnForwardPacket(sc);
    }
  }
  return IO_ERROR;
}

_io_op_result ULSBusConnectionsList::cnSendGetObject(uint8_t *route, uint8_t hs,
                                                     uint16_t obj_addr) {
  begin();
  while (next()) {
    if (current->cnSendGetObject(route, hs, obj_addr) == IO_OK) return IO_OK;
  }
  return IO_ERROR;
}
_io_op_result ULSBusConnectionsList::cnSendSetObject(uint8_t *route, uint8_t hs,
                                                     uint16_t obj_addr,
                                                     uint8_t *buf,
                                                     uint32_t size) {
  begin();
  while (next()) {
    if (current->cnSendSetObject(route, hs, obj_addr, buf, size) == IO_OK)
      return IO_OK;
  }
  return IO_ERROR;
}
_io_op_result ULSBusConnectionsList::cnSendExplorer() {
  begin();
  while (next()) {
    if (current->cnSendExplorer() != IO_OK) return IO_ERROR;
  }
  return IO_OK;
}
_io_op_result ULSBusConnectionsList::open() {
  begin();
  while (next()) {
    current->open();
  }
  return IO_OK;
}
void ULSBusConnectionsList::close() {
  begin();
  while (next()) {
    current->close();
  }
}
_io_op_result ULSBusConnectionsList::setDID(uint8_t cid, uint8_t did) {
  begin();
  while (next()) {
    if (current->cid() == cid) current->ifid(did);
  }
  return IO_OK;
}

_io_op_result ULSBusConnectionsList::cnSendSysSetMode(uint8_t *route,
                                                      uint8_t hs,
                                                      _cn_sys_mode mode) {
  begin();
  while (next()) {
    if (current->cnSendSysSetMode(route, hs, mode) == IO_OK) return IO_OK;
  }
  return IO_ERROR;
}
_io_op_result ULSBusConnectionsList::cnSendSysErase(uint8_t *route, uint8_t hs,
                                                    uint32_t key,
                                                    uint32_t start,
                                                    uint32_t len) {
  begin();
  while (next()) {
    if (current->cnSendSysErase(route, hs, key, start, len) == IO_OK)
      return IO_OK;
  }
  return IO_ERROR;
}
_io_op_result ULSBusConnectionsList::cnSendSysWrite(uint8_t *route, uint8_t hs,
                                                    uint32_t key,
                                                    uint32_t start,
                                                    uint32_t len,
                                                    uint8_t *buf) {
  begin();
  while (next()) {
    if (current->cnSendSysWrite(route, hs, key, start, len, buf) == IO_OK)
      return IO_OK;
  }
  return IO_ERROR;
}
_io_op_result ULSBusConnectionsList::cnSendSysSetSignature(
    uint8_t *route, uint8_t hs, uint32_t key, char *fw, char *ldr,
    uint32_t ftime, uint32_t progsize, uint32_t progcrc) {
  begin();
  while (next()) {
    if (current->cnSendSysSetSignature(route, hs, key, fw, ldr, ftime, progsize,
                                       progcrc) == IO_OK)
      return IO_OK;
  }
  return IO_ERROR;
}
_io_op_result ULSBusConnectionsList::cnSetClbkSys(_uls_cn_sys_callback func) {
  begin();
  while (next()) {
    current->cnclbkSys = func;
  }
  return IO_OK;
}
_io_op_result ULSBusConnectionsList::cnSetClbkSysAck(
    _uls_cn_sysack_callback func) {
  begin();
  while (next()) {
    current->cnclbkSysAck = func;
  }
  return IO_OK;
}
_io_op_result ULSBusConnectionsList::cnSendSysSaveConfig(uint8_t *route,
                                                         uint8_t hs,
                                                         uint32_t key) {
  begin();
  while (next()) {
    if (current->cnSendSysSaveConfig(route, hs, key) == IO_OK) return IO_OK;
  }
  return IO_ERROR;
}
