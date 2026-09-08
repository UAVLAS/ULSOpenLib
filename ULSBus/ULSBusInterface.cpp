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

#include "ULSBusInterface.h"

#define __DEVICE_KEY_BYTE                                 \
  ((__DEVICE_KEY & 0xff) ^ ((__DEVICE_KEY >> 8) & 0xff) ^ \
   ((__DEVICE_KEY >> 16) & 0xff) ^ ((__DEVICE_KEY >> 24) & 0xff))

ULSBusInterface::ULSBusInterface(const char* name, uint8_t did)
    : ifclbkBlitzReceived(nullptr),
      _name(name),
      _did(did),
      _static_did(255),
      _state(IF_STATE_UNINITIALIZED),
      _key(0),
      _key_cntr(0),
      _didx(0),
      _nm_timeout(0),
      _nm_requests(0)
#if defined(ULSBUS_AUTH)
      ,
      _authSecretClbk(nullptr),
      _authLevel(ULS_AUTH_LEVEL_NONE),
      _authPeerUid(0),
      _authTimeout(0),
      _authLockout(0),
      _authAttempts(0)
#endif
{
#if defined(ULSBUS_AUTH)
  _authPolicy.required = ULS_AUTH_LEVEL_NONE;  // open, as it always was
  _authPolicy.grantLevel = ULS_AUTH_LEVEL_NONE;
  memset(_authNonce, 0, sizeof(_authNonce));
#endif
  ifRxLen = 0;
  ifTxLen = 0;
  ifRxPacket = (_if_packet*)ifRxBuf;
  ifTxPacket = (_if_packet*)ifTxBuf;

  if (_did > 0x3f) _did = 0x3f;

  memset(ifRxBuf, 0, IF_PACKET_SIZE);
  memset(ifTxBuf, 0, IF_PACKET_SIZE);
  memset(_locals, 0, IF_LOCAL_DEVICES_NUM * (sizeof(_local_device)));
}
void ULSBusInterface::task(uint32_t dtms) {
  if ((_static_did < 63) && (_static_did > 0)) {
    _did = _static_did;
  }
  _key_cntr++;
#if defined(ULSBUS_AUTH)
  if (_authTimeout) {
    _authTimeout = (_authTimeout > dtms) ? (_authTimeout - dtms) : 0;
    if (_authTimeout == 0) {
      /* A join that stalled half way. Drop it rather than hold the slot: on a
       * policed interface an abandoned handshake is the cheap way in. */
      DEBUG_MSG("%s: AUTH timeout", _name);
      authReset();
      _state = IF_STATE_ERROR;
    }
  }
  if (_authLockout) {
    _authLockout = (_authLockout > dtms) ? (_authLockout - dtms) : 0;
  }
#endif
  if (_nm_timeout >= dtms) {
    _nm_timeout -= dtms;
  } else {
    _nm_timeout = 0;
  }
  // Check for timeout for devices
  /*
   * IF_STATE_AUTH is the one state where having no id yet is correct rather
   * than a fault: the id is what the handshake is being run to earn. Without
   * the exception this line drops the joiner back to UNINITIALIZED between the
   * challenge and the SET_ID that answers it, and processNM_SETID then refuses
   * the id because the interface is no longer in AUTH - so a correct exchange
   * never completes.
   */
  if ((_did >= 0x3f) && (_state != IF_STATE_AUTH))
    _state = IF_STATE_UNINITIALIZED;
  switch (_state) {
    case IF_STATE_UNINITIALIZED:
      if (_did < 0x3f) {
        _state = IF_STATE_OK;
        ifOk();
      } else {
        if (_nm_timeout == 0) {  // Request ID each IF_NM_REQUESTID_TIMEOUT
          if (_nm_requests < 5) {
#if defined(ULSBUS_AUTH)
            if (ifAuthRequired()) {
              sendAUTH_REQUEST();
            } else {
              sendNM_REQUESTID();
            }
#else
            sendNM_REQUESTID();
#endif
            _nm_requests++;
          } else if (ifAuthRequired()) {
            /* The self-assign fallback below picks a free id when no master
             * answers. On a policed interface that would be a way in without
             * ever authenticating, so it is not available here: no master, no
             * id, and the interface stays mute. */
            _nm_requests = 0;
            _nm_timeout = IF_NM_REQUESTID_TIMEOUT + randomTimeout();
          } else {  // Select random did if no master detected
            uint32_t n = IF_LOCAL_DEVICES_NUM - 2;  // NO master & broadcast
            uint8_t id = 0;
            while (n > 0U) {
              id++;
              if (id > IF_LOCAL_DEVICES_NUM) id = 1;
              if (_locals[id].timeout == 0) {  // slot emmpty
                _did = id;
                _state = IF_STATE_OK;
                ifOk();
                break;
              }
              n--;
            }
            _nm_requests = 0;
          }
          _nm_timeout = IF_NM_REQUESTID_TIMEOUT + randomTimeout();
        }
      }
      break;
    case IF_STATE_OK:
      if (_nm_timeout == 0) {
        sendNM_HB();
        _nm_timeout = IF_NM_PING_HB_TIMEOUT;
      }
      for (uint32_t i = 0; i < IF_LOCAL_DEVICES_NUM; i++) {
        if (_locals[i].timeout >= dtms) {
          _locals[i].timeout -= dtms;
          if (_locals[i].timeout < dtms) {
            deviceDisconnected(i);
          }
        } else {
          _locals[i].timeout = 0;
        }
      }

      break;
    case IF_STATE_AUTH:
      /* Challenged and waiting. Nothing to send: _authTimeout above is what
       * gets us out if the other side goes quiet. Upper-layer traffic is
       * already blocked, because send() and receive() both test for
       * IF_STATE_OK and this is not it. */
      break;
    case IF_STATE_ERROR:
      // Check error and go to IF_STATE_UNINITIALIZED
      _nm_timeout = 0;
      _state = IF_STATE_UNINITIALIZED;
      break;
  }
}
_io_op_result ULSBusInterface::ifSetStaticDID(uint8_t static_did) {
  if (static_did >= 0x3f || static_did == 0) {
    return IO_ERROR;
  }
  _static_did = static_did;
  return IO_OK;
}
_io_op_result ULSBusInterface::ifSend() {
  ifTxPacket->cmd |= 0x10;
  return send();
}
_io_op_result ULSBusInterface::ifReceive() {
  _io_op_result rez = receive();
  if (rez == IO_OK) {
    ifRxPacket->cmd &= 0x0f;
  }
  return rez;
}
_io_op_result ULSBusInterface::send() {
  if ((_state != IF_STATE_OK) && ((ifTxPacket->cmd & 0x10) != 0))
    return IO_ERROR;  // Only sys commands allowed
  ifTxPacket->src_lid = _did;
  ifTxLen += IF_PACKET_HEADER_SIZE;
  DEBUG_MSG("%s: Send lid:0x%.2X cmd: 0x%.2X len: %d", _name, _did,
            ifTxPacket->cmd, ifTxLen);
  if (ifTxPacket->cmd != IF_CMD_NM_HB) {
    DEBUG_PACKET(_name, "Tx", ifTxBuf, ifTxLen);
  }
  _io_op_result rez = sendPacket();
  if (rez == IO_OK) _nm_timeout = IF_NM_PING_HB_TIMEOUT;
  return rez;
}
_io_op_result ULSBusInterface::receive() {
  while (ifRxLen == 0) {
    _io_op_result rez = receivePacket();
    if (rez != IO_OK) return rez;
    DEBUG_MSG("%s: Received lid: 0x%.2X cmd: 0x%.2X len: %d", _name,
              ifRxPacket->src_lid, ifRxPacket->cmd, ifRxLen);
    if (ifRxPacket->cmd != IF_CMD_NM_HB) {
      DEBUG_PACKET(_name, "Rx", ifRxBuf, ifRxLen);
    }
    ifRxLen -= IF_PACKET_HEADER_SIZE;
    if (ifRxPacket->src_lid == _did) {  // error duplicate address
      resetId();
      ifRxLen = 0;
      return IO_NO_DATA;
    }
    /*
     * Local-device state used to be created here for every packet received,
     * before anything had been checked. On a policed interface that let an
     * unauthenticated peer fill all 63 slots by varying src_lid, which is the
     * same denial the authorization is meant to prevent, one layer down. So on
     * such an interface no slot exists until the link is up.
     */
    if ((!ifAuthRequired()) || (_state == IF_STATE_OK)) {
      // if it was disconnected call procedure
      if (_locals[ifRxPacket->src_lid].timeout == 0) {
        if (ifRxPacket->src_lid <= 0x3f) deviceConnected(ifRxPacket->src_lid);
      }
      // Update Timeout of device
      _locals[ifRxPacket->src_lid].timeout = IF_NM_DVICE_HB_TIMEOUT;
    }

    if ((ifRxPacket->cmd & 0x10) == 0) {  // Process NM and SYS packets
      processLocal();
      ifRxLen = 0;
      continue;
    }
    if (_state != IF_STATE_OK) {  // Skip all other packets if we are not OK
      ifRxLen = 0;
    }
  }
  return IO_OK;
}
void ULSBusInterface::processLocal() {
  switch (ifRxPacket->cmd) {
    case IF_CMD_SYS:
      break;
    case IF_CMD_NM_GET_STATUS:
      break;
    case IF_CMD_NM_HB:
      if (ifRxLen == IF_PACKET_NM_HB_SIZE) processNM_HB();
      break;
    case IF_CMD_NM_REQUEST_ID:
      /* An unauthenticated join. Refused outright on a policed interface -
       * this is the legacy path, and it proves nothing. */
      if ((ifRxLen == IF_PACKET_NM_REQUESTID_SIZE) && (_did == 0x0) &&
          (!ifAuthRequired()))
        sendNM_SETID(ifRxPacket->request_id.key);
      break;
#if defined(ULSBUS_AUTH)
    case IF_CMD_NM_AUTH_REQUEST:
      if ((ifRxLen == IF_PACKET_AUTH_REQUEST_SIZE) && (_did == 0x0))
        processAUTH_REQUEST();
      break;
    case IF_CMD_NM_AUTH_CHALLENGE:
      if (ifRxLen == IF_PACKET_AUTH_CHALLENGE_SIZE) processAUTH_CHALLENGE();
      break;
    case IF_CMD_NM_AUTH_RESPONSE:
      if ((ifRxLen == IF_PACKET_AUTH_RESPONSE_SIZE) && (_did == 0x0))
        processAUTH_RESPONSE();
      break;
    case IF_CMD_NM_AUTH_REJECT:
      if (ifRxLen == IF_PACKET_AUTH_REJECT_SIZE) processAUTH_REJECT();
      break;
#endif
    case IF_CMD_NM_SET_ID:
      if (ifRxLen == IF_PACKET_NM_SETID_SIZE) processNM_SETID();
      break;
    case IF_CMD_NM_RESET_ID:

      break;
    case IF_CMD_BLITZ:
      IF_CALL(ifclbkBlitzReceived);
      break;
  }
  ifRxLen = 0;
}

_io_op_result ULSBusInterface::sendNM_REQUESTID() {
  ifTxPacket->cmd = IF_CMD_NM_REQUEST_ID;
  ifTxLen = IF_PACKET_NM_REQUESTID_SIZE;

  _key = __DEVICE_KEY ^ _key_cntr;
  DEBUG_MSG("%s: send REQUEST ID lid:0x%.2X KEY: 0x%.4X ", _name, _did, _key);
  ifTxPacket->request_id.key = _key;
  return send();
}
_io_op_result ULSBusInterface::sendNM_HB() {
  ifTxPacket->cmd = IF_CMD_NM_HB;
  ifTxLen = IF_PACKET_NM_HB_SIZE;

  ifTxPacket->hb.uid0 = __DEVICE_UNIC_ID0;
  return send();
}
_io_op_result ULSBusInterface::sendNM_SETID(uint32_t key) {
  ifTxPacket->cmd = IF_CMD_NM_SET_ID;
  ifTxLen = IF_PACKET_NM_SETID_SIZE;

  ifTxPacket->set_id.new_id = allocateId();
  DEBUG_MSG(
      "%s: send SET ID lid:0x%.2X NEWID: 0x%.2X KEY:\
   0x%.4X",
      _name, _did, ifTxPacket->set_id.new_id, key);
  ifTxPacket->set_id.key = key;
  return send();
}
_io_op_result ULSBusInterface::ifSendBLITZ(uint16_t blitz_msg_id, uint8_t* buf,
                                           uint32_t size) {
  if (_state != IF_STATE_OK) return IO_ERROR;
  if (size > 8) return IO_ERROR;
  ifTxPacket->cmd = IF_CMD_BLITZ;
  ifTxLen = IF_PACKET_BLITZ_SIZE + size;
  ifTxPacket->blitz.msg_id = blitz_msg_id;
  memcpy(ifTxPacket->blitz.data, buf, size);
  return send();
}

uint8_t ULSBusInterface::allocateId() {
  uint32_t n = IF_LOCAL_DEVICES_NUM;  // 0 - master device excluded
  while (n > 0U) {
    _didx++;
    if (_didx == 0) _didx++;
    if (_didx >= IF_LOCAL_DEVICES_NUM)
      _didx = 1;                        // O not allocated - it is master
    if (_locals[_didx].timeout == 0) {  // slot emmpty - no device connected
      return _didx;
    }
    n--;
  }
  return IF_LOCAL_DEVICES_NUM;
}
uint8_t ULSBusInterface::randomTimeout() {
  uint32_t uidshift = (_key_cntr / 4) % 32;
  switch (_key_cntr % 4) {
    case 0:
      return ((__DEVICE_UNIC_ID0 >> uidshift) & 0xff);
      break;
    case 1:
      return ((__DEVICE_UNIC_ID1 >> uidshift) & 0xff);
      break;
    case 2:
      return ((__DEVICE_UNIC_ID2 >> uidshift) & 0xff);
      break;
    case 3:
      return ((__DEVICE_UNIC_ID3 >> uidshift) & 0xff);
      break;
  }
  return 0;
}
void ULSBusInterface::processNM_SETID() {
  DEBUG_MSG(
      "%s: Received SET ID new ID:0x%.2X REMKEY:0x%.4X  OUR KEY:\
   0x%.4X",
      _name, ifTxPacket->set_id.new_id, ifRxPacket->set_id.key, _key);
  if (ifRxPacket->set_id.key != _key) return;
  /* On a policed interface an id is only ever accepted as the last step of a
   * handshake we ourselves started and whose challenge we already answered.
   * Without this an unsolicited SET_ID would skip the authorization entirely. */
  if (ifAuthRequired() && (_state != IF_STATE_AUTH)) {
    DEBUG_MSG("%s: SET ID refused - not in AUTH", _name);
    return;
  }
  _did = ifRxPacket->set_id.new_id;
#if defined(ULSBUS_AUTH)
  _authTimeout = 0;
#endif
  _state = IF_STATE_OK;
  sendNM_HB();  // Send PING answer;
  ifOk();
}
void ULSBusInterface::processNM_HB() {
  uint8_t idx = ifRxPacket->src_lid;
  _locals[idx].uid0 = ifRxPacket->hb.uid0;
}
void ULSBusInterface::resetId() {
  _did = IF_LOCAL_DEVICES_NUM;
  _state = IF_STATE_ERROR;
#if defined(ULSBUS_AUTH)
  /* A link that dropped and came back is not the same link. Re-authenticate:
   * carrying a grant across would let it outlive the peer that earned it. */
  authReset();
#endif
}

void ULSBusInterface::processSYS() {}

#if defined(ULSBUS_AUTH)
/* ------------------------------------------------------------------------
 * Authorization
 *
 * Admission control, one layer below routing. A peer that does not complete
 * this never reaches IF_STATE_OK, and send() and receive() already refuse
 * every upper-layer packet in that state - objects, sys, and explorer alike.
 * Gating explorer is the point: cnProcessExplorer floods every other
 * connection on the node, so one frame accepted from a stranger on a radio
 * becomes a traversal of the whole internal bus.
 *
 *   joiner  --AUTH_REQUEST(nonceA, uid, level)-->  master
 *   joiner  <--AUTH_CHALLENGE(nonceB, proofM)---   master   (no id yet)
 *   joiner  --AUTH_RESPONSE(proofJ)------------->  master
 *   joiner  <--SET_ID(new_id, key echoed)-------   master   (id allocated now)
 *
 * Mutual, and at no extra round trip: the master proves itself in the same
 * message that carries its challenge. Without that a fake master collects a
 * valid response from every device that tries to join it.
 * ------------------------------------------------------------------------ */

void ULSBusInterface::authReset() {
  _authLevel = ULS_AUTH_LEVEL_NONE;
  _authPeerUid = 0;
  _authTimeout = 0;
  memset(_authNonce, 0, sizeof(_authNonce));
}

_io_op_result ULSBusInterface::ifSetAuthPolicy(uint8_t required,
                                               uint8_t grantLevel,
                                               _uls_if_secret_callback secret) {
  if ((required != ULS_AUTH_LEVEL_NONE) && (secret == nullptr))
    return IO_ERROR;  // demanding a proof with no way to check it
  _authPolicy.required = required;
  _authPolicy.grantLevel = grantLevel;
  _authSecretClbk = secret;
  authReset();
  return IO_OK;
}

bool ULSBusInterface::authSecret(uint32_t joinerUid, uint8_t level,
                                 uint8_t key[ULS_AES_KEY_SIZE]) {
  if (_authSecretClbk == nullptr) return false;
  return _authSecretClbk(level, joinerUid, key);
}

/*
 * proof = CMAC(secret, who || nonce || uid || level)
 *
 * 'who' is 'M' or 'J'. Without it the two directions compute the same value
 * over the same inputs, and a proof could be reflected back at the side that
 * sent it.
 */
void ULSBusInterface::authProof(const uint8_t key[ULS_AES_KEY_SIZE], char who,
                                const uint8_t nonce[16], uint32_t uid,
                                uint8_t level, uint8_t proof[16]) {
  uint8_t buf[1 + 16 + 4 + 1];
  buf[0] = (uint8_t)who;
  memcpy(&buf[1], nonce, 16);
  buf[17] = (uint8_t)(uid);
  buf[18] = (uint8_t)(uid >> 8);
  buf[19] = (uint8_t)(uid >> 16);
  buf[20] = (uint8_t)(uid >> 24);
  buf[21] = level;
  ulsCryptoCmac(key, buf, sizeof(buf), proof);
}

/* Joiner: start a policed join. */
_io_op_result ULSBusInterface::sendAUTH_REQUEST() {
  if (!ULS_CRYPTO_RANDOM(_authNonce, 16)) {
    /* No entropy means no unpredictable nonce, and a predictable nonce lets a
     * recorded exchange be replayed. Refusing to join is the safe failure. */
    DEBUG_MSG("%s: AUTH no random source", _name);
    return IO_ERROR;
  }
  _key = __DEVICE_KEY ^ _key_cntr;  // request tag, as REQUEST_ID uses

  ifTxPacket->cmd = IF_CMD_NM_AUTH_REQUEST;
  ifTxLen = IF_PACKET_AUTH_REQUEST_SIZE;
  ifTxPacket->auth_request.key = _key;
  ifTxPacket->auth_request.uid = __DEVICE_UNIC_ID0;
  ifTxPacket->auth_request.level = _authPolicy.required;
  memcpy(ifTxPacket->auth_request.nonce, _authNonce, 16);

  DEBUG_MSG("%s: send AUTH REQUEST level:%d", _name, _authPolicy.required);
  return send();
}

_io_op_result ULSBusInterface::sendAUTH_REJECT(uint8_t reason) {
  ifTxPacket->cmd = IF_CMD_NM_AUTH_REJECT;
  ifTxLen = IF_PACKET_AUTH_REJECT_SIZE;
  ifTxPacket->auth_reject.reason = reason;
  DEBUG_MSG("%s: send AUTH REJECT reason:%d", _name, reason);
  return send();
}

/* Master: challenge the joiner. No id is allocated here - allocateId() runs
 * only in processAUTH_RESPONSE, after the proof checks out. */
void ULSBusInterface::processAUTH_REQUEST() {
  if (!ifAuthRequired()) return;  // open interface, nothing to prove
  if (_authLockout) return;       // too many bad answers lately, stay quiet

  uint8_t level = ifRxPacket->auth_request.level;
  uint32_t peerUid = ifRxPacket->auth_request.uid;
  uint8_t joinerNonce[16];
  memcpy(joinerNonce, ifRxPacket->auth_request.nonce, 16);

  if (level != _authPolicy.required) {
    sendAUTH_REJECT(IF_AUTH_REJECT_LEVEL);
    return;
  }

  uint8_t key[ULS_AES_KEY_SIZE];
  if (!authSecret(peerUid, level, key)) {
    sendAUTH_REJECT(IF_AUTH_REJECT_NOSECRET);
    return;
  }

  if (!ULS_CRYPTO_RANDOM(_authNonce, 16)) {
    sendAUTH_REJECT(IF_AUTH_REJECT_NORANDOM);
    return;
  }

  /* Our proof to the joiner, over the nonce it chose. */
  uint8_t proofM[16];
  authProof(key, 'M', joinerNonce, __DEVICE_UNIC_ID0, level, proofM);
  memset(key, 0, sizeof(key));

  _authPeerUid = peerUid;
  _key = ifRxPacket->auth_request.key;  // echoed back in SET_ID at the end
  _authTimeout = IF_AUTH_TIMEOUT_MS;

  ifTxPacket->cmd = IF_CMD_NM_AUTH_CHALLENGE;
  ifTxLen = IF_PACKET_AUTH_CHALLENGE_SIZE;
  ifTxPacket->auth_challenge.uid = __DEVICE_UNIC_ID0;
  ifTxPacket->auth_challenge.level = level;
  memcpy(ifTxPacket->auth_challenge.nonce, _authNonce, 16);
  memcpy(ifTxPacket->auth_challenge.proof, proofM, 16);

  DEBUG_MSG("%s: AUTH challenge sent to uid:0x%.8X", _name,
            (unsigned int)peerUid);
  send();
}

/* Joiner: check the master, then answer its challenge. */
void ULSBusInterface::processAUTH_CHALLENGE() {
  if (!ifAuthRequired()) return;
  if (_state == IF_STATE_OK) return;  // already in, nothing to answer

  uint8_t level = ifRxPacket->auth_challenge.level;
  uint32_t masterUid = ifRxPacket->auth_challenge.uid;

  uint8_t key[ULS_AES_KEY_SIZE];
  if (!authSecret(__DEVICE_UNIC_ID0, level, key)) {
    DEBUG_MSG("%s: AUTH no secret for level %d", _name, level);
    return;
  }

  /* Is this master genuine? It answered over the nonce we chose, so a
   * recorded challenge from an earlier join will not verify here. */
  uint8_t expect[16];
  authProof(key, 'M', _authNonce, masterUid, level, expect);
  if (!ulsCryptoEqual(expect, ifRxPacket->auth_challenge.proof, 16)) {
    DEBUG_MSG("%s: AUTH master proof BAD", _name);
    memset(key, 0, sizeof(key));
    authReset();
    _state = IF_STATE_ERROR;
    return;
  }

  uint8_t proofJ[16];
  authProof(key, 'J', ifRxPacket->auth_challenge.nonce, __DEVICE_UNIC_ID0,
            level, proofJ);
  memset(key, 0, sizeof(key));

  _authLevel = level;
  _authTimeout = IF_AUTH_TIMEOUT_MS;
  _state = IF_STATE_AUTH;

  ifTxPacket->cmd = IF_CMD_NM_AUTH_RESPONSE;
  ifTxLen = IF_PACKET_AUTH_RESPONSE_SIZE;
  memcpy(ifTxPacket->auth_response.proof, proofJ, 16);

  DEBUG_MSG("%s: AUTH master OK, answering", _name);
  send();
}

/* Master: check the joiner, and only now hand out an id. */
void ULSBusInterface::processAUTH_RESPONSE() {
  if (!ifAuthRequired()) return;
  if (_authTimeout == 0) return;  // nothing outstanding
  if (_authLockout) return;

  uint8_t key[ULS_AES_KEY_SIZE];
  if (!authSecret(_authPeerUid, _authPolicy.required, key)) {
    sendAUTH_REJECT(IF_AUTH_REJECT_NOSECRET);
    authReset();
    return;
  }

  uint8_t expect[16];
  authProof(key, 'J', _authNonce, _authPeerUid, _authPolicy.required, expect);
  memset(key, 0, sizeof(key));

  if (!ulsCryptoEqual(expect, ifRxPacket->auth_response.proof, 16)) {
    _authAttempts++;
    DEBUG_MSG("%s: AUTH joiner proof BAD (%d)", _name, _authAttempts);
    if (_authAttempts >= IF_AUTH_MAX_ATTEMPTS) {
      _authLockout = IF_AUTH_LOCKOUT_MS;
      _authAttempts = 0;
    }
    sendAUTH_REJECT(IF_AUTH_REJECT_BADPROOF);
    authReset();
    return;
  }

  _authAttempts = 0;
  _authLevel = _authPolicy.grantLevel;
  _authTimeout = 0;

  DEBUG_MSG("%s: AUTH uid:0x%.8X granted level %d", _name,
            (unsigned int)_authPeerUid, _authLevel);
  sendNM_SETID(_key);
}

void ULSBusInterface::processAUTH_REJECT() {
  /* Told no, rather than left to time out. The distinction matters to a PC:
   * "this device needs a password" is a different message from "no device". */
  DEBUG_MSG("%s: AUTH rejected, reason %d", _name,
            ifRxPacket->auth_reject.reason);
  authReset();
  _state = IF_STATE_ERROR;
}
#endif  // ULSBUS_AUTH
