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

#ifndef ULSBUSINTERFACE_H
#define ULSBUSINTERFACE_H

#include "ULSBusTypes.h"
#include "ULSCrypto.h"
#include "ULSDevices.h"

#define IF_PACKET_SIZE 1324

#define IF_PACKET_HEADER_SIZE (2)
#define IF_PAYLOAD_SIZE (IF_PACKET_SIZE - IF_PACKET_HEADER_SIZE)

#define IF_NM_DVICE_HB_TIMEOUT 3000
#define IF_NM_PING_HB_TIMEOUT 1000
#define IF_NM_REQUESTID_TIMEOUT 100

#define IF_LOCAL_DEVICES_NUM 63  // local devices 0-62 , 63 bradcast device

#define IF_PACKET_SYS_SIZE (2)
#define IF_PACKET_NM_HEADER_SIZE (0)
#define IF_PACKET_NM_HB_SIZE (IF_PACKET_NM_HEADER_SIZE + 4)
#define IF_PACKET_NM_REQUESTID_SIZE (IF_PACKET_NM_HEADER_SIZE + 4)
#define IF_PACKET_NM_SETID_SIZE (IF_PACKET_NM_HEADER_SIZE + 1 + 4)
#define IF_PACKET_BLITZ_SIZE (2)
#define IF_PACKET_FORCELDR_SIZE (2)

/* Authorization. Payload sizes exclude the 2-byte interface header, like the
 * NM sizes above, because processLocal dispatches on ifRxLen after it has been
 * subtracted. */
#define IF_PACKET_AUTH_REQUEST_SIZE (4 + 4 + 1 + 16)
#define IF_PACKET_AUTH_CHALLENGE_SIZE (4 + 1 + 16 + 16)
#define IF_PACKET_AUTH_RESPONSE_SIZE (16)
#define IF_PACKET_AUTH_REJECT_SIZE (1)

/* How long the joiner has to answer a challenge, and how many bad answers an
 * interface accepts before it stops replying for a while. Both matter only on
 * a policed interface, where an attacker retries as fast as the radio allows
 * and every attempt costs the master a CMAC. */
#define IF_AUTH_TIMEOUT_MS 3000
#define IF_AUTH_MAX_ATTEMPTS 5
#define IF_AUTH_LOCKOUT_MS 10000

class ULSBusInterface;
typedef void (*_uls_if_callback)(ULSBusInterface *);

#define IF_CALL(func) \
  if (func != nullptr) func(this);

typedef enum {
  IO_OK = 0,
  IO_NO_DATA,
  IO_BUFFER_FULL,
  IO_ERROR,
  IO_ERROR_REMOTE_HOST_NOT_CONNECTED,
  IO_HOST_NOTFOUND
} _io_op_result;
typedef enum {
  IF_CMD_SYS = 0,
  IF_CMD_NM_HB = 1,
  IF_CMD_NM_GET_STATUS = 2,
  IF_CMD_NM_REQUEST_ID = 3,
  IF_CMD_NM_SET_ID = 4,
  IF_CMD_NM_RESET_ID = 5,
  IF_CMD_BLITZ = 6,
  /* Authorization. Values 7-15 are free: ifReceive masks cmd with 0x0f and
   * 0x10 marks an upper-layer packet, so anything below 16 stays at layer 1
   * and is handled by processLocal without ever reaching the connection. */
  IF_CMD_NM_AUTH_REQUEST = 7,
  IF_CMD_NM_AUTH_CHALLENGE = 8,
  IF_CMD_NM_AUTH_RESPONSE = 9,
  IF_CMD_NM_AUTH_REJECT = 10
} _if_cmd;

/*
 * What a peer proved. Granted at layer 1 on admission and readable from layer
 * 2 without any plumbing, because ULSBusConnection inherits ULSBusInterface.
 */
typedef enum : uint8_t {
  ULS_AUTH_LEVEL_NONE = 0,
  ULS_AUTH_LEVEL_PEER = 1,      /* another device of the same fleet */
  ULS_AUTH_LEVEL_OPERATOR = 2,  /* a PC holding the operator secret */
  ULS_AUTH_LEVEL_ADMIN = 3      /* firmware, factory, sysconfig */
} _uls_auth_level;

typedef enum : uint8_t {
  IF_AUTH_REJECT_BADPROOF = 0,
  IF_AUTH_REJECT_NOSECRET = 1,
  IF_AUTH_REJECT_LEVEL = 2,
  IF_AUTH_REJECT_BUSY = 3,
  IF_AUTH_REJECT_NORANDOM = 4
} _if_auth_reject;

/*
 * Per-interface policy. All zero is exactly today's behaviour, which is what
 * every existing device gets by default: no authorization, nothing to notice.
 * Only an interface that opts in demands anything.
 */
typedef struct {
  uint8_t required;    /* _uls_auth_level to join; 0 = open interface */
  uint8_t grantLevel;  /* what a successful join is granted */
} _if_auth_policy;

/*
 * Supplies the shared secret for a join. Always keyed on the JOINER: the
 * master looks it up by the peer uid it was sent, the joiner by its own, and
 * the two agree because the derivation names one device, not two.
 *
 * Return false to refuse - an unknown peer, a level this device will not
 * grant, or no fleet key provisioned.
 */
typedef bool (*_uls_if_secret_callback)(uint8_t level, uint32_t joinerUid,
                                        uint8_t key[ULS_AES_KEY_SIZE]);

typedef enum {
  IF_STATE_UNINITIALIZED = 0,
  IF_STATE_OK = 1,
  IF_STATE_AUTH = 2, /* challenged, waiting - value 2 was already free */
  IF_STATE_ERROR = 3,
} _if_state;

// Packet structure
typedef __ULS_PACKET(struct {
  uint8_t cmd;
  uint8_t src_lid;
  __ULS_PACKET(union {
    // Sysmem messges
    __ULS_PACKET(struct {
      uint8_t dsn_lid;
      uint8_t param;
    })
    ys;
    // Network messges
    __ULS_PACKET(union {
      // HB packets
      __ULS_PACKET(struct { uint32_t uid0; }) hb;
      // status
      __ULS_PACKET(struct { uint8_t dsn_lid; }) get_status;
      // Request ID
      __ULS_PACKET(struct { uint32_t key; }) request_id;
      // Set Id
      __ULS_PACKET(struct {
        uint8_t new_id;
        uint32_t key;
      })
      set_id;
      // Authorization
      __ULS_PACKET(struct {
        uint32_t key; /* echoed back in SET_ID, as REQUEST_ID's is */
        uint32_t uid; /* the joiner's uid0 - whose secret this is */
        uint8_t level;
        uint8_t nonce[16];
      })
      auth_request;
      __ULS_PACKET(struct {
        uint32_t uid; /* the master's uid0, so the joiner can check proof */
        uint8_t level;
        uint8_t nonce[16];
        uint8_t proof[16];
      })
      auth_challenge;
      __ULS_PACKET(struct { uint8_t proof[16]; }) auth_response;
      __ULS_PACKET(struct { uint8_t reason; }) auth_reject;
    });
    // Blitzh messges
    __ULS_PACKET(struct {
      uint16_t msg_id;
      uint8_t data[8];
    })
    blitz;
    // Buffer direct access
    uint8_t pld[IF_PAYLOAD_SIZE];  // Payload of if_packet
  });
}) _if_packet;

typedef struct {
  // uint8_t  id;
  uint32_t timeout;
  uint32_t uid0;
} _local_device;

class ULSBusInterface {
 public:
  ULSBusInterface(const char *name = nullptr,
                  uint8_t did = IF_LOCAL_DEVICES_NUM);
  virtual bool open() { return false; };
  virtual void close() { ; };
  void task(uint32_t dtms);

  _io_op_result ifSend();
  _io_op_result ifReceive();
  _io_op_result ifSendBLITZ(uint16_t blitz_msg_id, uint8_t *buf, uint32_t size);
  _io_op_result ifSetStaticDID(uint8_t static_did);

  /*
   * Turn authorization on for this interface. Until this is called the
   * interface is open and behaves exactly as it always has.
   */
  _io_op_result ifSetAuthPolicy(uint8_t required, uint8_t grantLevel,
                                _uls_if_secret_callback secret);

  /* What the peer on this link proved. ULS_AUTH_LEVEL_NONE on an open
   * interface, which is the same value an unauthenticated peer has - an open
   * interface grants no level because it checked nothing. */
  uint8_t ifAuthLevel() { return _authLevel; }
  bool ifAuthRequired() { return _authPolicy.required != ULS_AUTH_LEVEL_NONE; }

  _uls_if_callback ifclbkBlitzReceived;

  uint8_t ifid() { return (_did & 0x3f); }
  void ifid(uint8_t did) { _did = (did & 0x3f); }

  const char *name() { return _name; };

  _if_packet *ifRxPacket;
  _if_packet *ifTxPacket;

  uint32_t ifRxLen;
  uint32_t ifTxLen;
  uint8_t ifRxBuf[IF_PACKET_SIZE];
  uint8_t ifTxBuf[IF_PACKET_SIZE];

 protected:
  virtual _io_op_result sendPacket() { return IO_ERROR; };
  virtual _io_op_result receivePacket() { return IO_ERROR; };
  virtual void deviceConnected(uint8_t id) { (void)id; };
  virtual void deviceDisconnected(uint8_t id) { (void)id; };
  virtual void ifOk() {};

 private:
  _io_op_result send();
  _io_op_result receive();
  // Process received packets
  void processLocal();
  void processSYS();
  void processNM_SETID();
  void processNM_HB();
  // Send packets
  _io_op_result sendNM_REQUESTID();
  _io_op_result sendNM_SETID(uint32_t key);
  _io_op_result sendNM_HB();
  // Authorization
  void processAUTH_REQUEST();
  void processAUTH_CHALLENGE();
  void processAUTH_RESPONSE();
  void processAUTH_REJECT();
  _io_op_result sendAUTH_REQUEST();
  _io_op_result sendAUTH_REJECT(uint8_t reason);
  bool authSecret(uint32_t joinerUid, uint8_t level,
                  uint8_t key[ULS_AES_KEY_SIZE]);
  void authProof(const uint8_t key[ULS_AES_KEY_SIZE], char who,
                 const uint8_t nonce[16], uint32_t uid, uint8_t level,
                 uint8_t proof[16]);
  void authReset();
  // Utils
  uint8_t allocateId();
  uint8_t randomTimeout();

  void resetId();

 protected:
  const char *_name;
  uint8_t _did;         // Local device Id
  uint8_t _static_did;  // Local device Id static settings

  ULSDBase *_dev;
  // uint64_t _iftime;
  _if_state _state;
  _local_device _locals[IF_LOCAL_DEVICES_NUM + 1];

  _if_auth_policy _authPolicy;
  _uls_if_secret_callback _authSecretClbk;
  uint8_t _authLevel;      /* what the peer on this link proved */
  uint8_t _authNonce[16];  /* ours: the challenge we sent, or the one we will
                            * be answered against */
  uint32_t _authPeerUid;   /* master side: who is mid-join */
  uint32_t _authTimeout;   /* countdown for IF_STATE_AUTH */
  uint32_t _authLockout;   /* set after too many bad answers */
  uint8_t _authAttempts;

 private:
  uint32_t _key;
  uint32_t _key_cntr;
  uint32_t _didx;
  uint32_t _nm_timeout;
  uint32_t _nm_requests;
};

#endif  // ULSBUSINTERFACE_H
