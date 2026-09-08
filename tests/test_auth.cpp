/* Two ULSBusInterface instances wired back to back, to prove the admission
 * handshake actually admits and actually refuses. */
#include <cstdio>
#include <cstring>
#include <deque>
#include <vector>
#include "ULSBusInterface.h"

unsigned int g_uid0 = 0;
static unsigned int rngCtr = 1;
bool hostRandom(unsigned char *buf, unsigned int len) {
  for (unsigned i = 0; i < len; i++) buf[i] = (unsigned char)(rngCtr * 31u + i);
  rngCtr++;
  return true;
}

struct Frame { std::vector<uint8_t> b; };
static std::deque<Frame> qA, qB;   /* qX = frames waiting for node X */

static const uint8_t FLEET[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
static const uint8_t WRONG[16] = {9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9};
static bool useWrongOnJoiner = false;
static bool joinerTurn = false;   /* which node is running right now */

static bool secretOk(uint8_t level, uint32_t joinerUid, uint8_t key[16]) {
  const uint8_t *fleet = (joinerTurn && useWrongOnJoiner) ? WRONG : FLEET;
  uint8_t ctx[5] = {(uint8_t)joinerUid, (uint8_t)(joinerUid>>8),
                    (uint8_t)(joinerUid>>16), (uint8_t)(joinerUid>>24), level};
  ulsCryptoDeriveKey(fleet, "uls-join", ctx, 5, key);
  return true;
}

class Node : public ULSBusInterface {
 public:
  Node(const char *n, uint8_t did, std::deque<Frame> *in, std::deque<Frame> *out)
      : ULSBusInterface(n, did), _in(in), _out(out) {}
  bool open() override { return true; }
  _if_state state() { return _state; }
  uint8_t did() { return _did; }
 protected:
  _io_op_result sendPacket() override {
    Frame f; f.b.assign(ifTxBuf, ifTxBuf + ifTxLen); _out->push_back(f);
    return IO_OK;
  }
  _io_op_result receivePacket() override {
    if (_in->empty()) return IO_NO_DATA;
    Frame f = _in->front(); _in->pop_front();
    memcpy(ifRxBuf, f.b.data(), f.b.size());
    ifRxLen = (uint32_t)f.b.size();
    return IO_OK;
  }
 private:
  std::deque<Frame> *_in, *_out;
};

static int fails = 0;
static void expect(const char *name, bool ok) {
  printf("%-42s %s\n", name, ok ? "PASS" : "FAIL");
  if (!ok) fails++;
}

/* Run both nodes for a while. 1 ms per step. */
static void run(Node &master, Node &joiner, int steps) {
  for (int i = 0; i < steps; i++) {
    joinerTurn = false; g_uid0 = 0xA0000001; master.task(1); master.ifReceive();
    joinerTurn = true;  g_uid0 = 0xB0000002; joiner.task(1); joiner.ifReceive();
  }
}

int main() {
  /* 1. open interfaces - nothing policed, the legacy path must still work */
  {
    qA.clear(); qB.clear(); rngCtr = 1;
    Node m("M", 0, &qA, &qB), j("J", 63, &qB, &qA);
    run(m, j, 400);
    expect("open: joiner joins (regression)", j.state() == IF_STATE_OK && j.did() != 63);
    expect("open: no level granted", j.ifAuthLevel() == ULS_AUTH_LEVEL_NONE);
  }

  /* 2. both policed, matching fleet key */
  {
    qA.clear(); qB.clear(); rngCtr = 1; useWrongOnJoiner = false;
    Node m("M", 0, &qA, &qB), j("J", 63, &qB, &qA);
    m.ifSetAuthPolicy(ULS_AUTH_LEVEL_OPERATOR, ULS_AUTH_LEVEL_OPERATOR, secretOk);
    j.ifSetAuthPolicy(ULS_AUTH_LEVEL_OPERATOR, ULS_AUTH_LEVEL_OPERATOR, secretOk);
    run(m, j, 400);
    expect("policed: joiner admitted", j.state() == IF_STATE_OK && j.did() != 63);
    expect("policed: master granted level", m.ifAuthLevel() == ULS_AUTH_LEVEL_OPERATOR);
    expect("policed: joiner recorded level", j.ifAuthLevel() == ULS_AUTH_LEVEL_OPERATOR);
  }

  /* 3. joiner holds the wrong fleet key */
  {
    qA.clear(); qB.clear(); rngCtr = 1; useWrongOnJoiner = true;
    Node m("M", 0, &qA, &qB), j("J", 63, &qB, &qA);
    m.ifSetAuthPolicy(ULS_AUTH_LEVEL_OPERATOR, ULS_AUTH_LEVEL_OPERATOR, secretOk);
    j.ifSetAuthPolicy(ULS_AUTH_LEVEL_OPERATOR, ULS_AUTH_LEVEL_OPERATOR, secretOk);
    run(m, j, 400);
    expect("wrong key: joiner refused", j.state() != IF_STATE_OK && j.did() == 63);
    expect("wrong key: master grants nothing", m.ifAuthLevel() == ULS_AUTH_LEVEL_NONE);
    useWrongOnJoiner = false;
  }

  /* 4. legacy joiner against a policed master - the unauthenticated path */
  {
    qA.clear(); qB.clear(); rngCtr = 1;
    Node m("M", 0, &qA, &qB), j("J", 63, &qB, &qA);
    m.ifSetAuthPolicy(ULS_AUTH_LEVEL_OPERATOR, ULS_AUTH_LEVEL_OPERATOR, secretOk);
    /* joiner left open: it will send the legacy REQUEST_ID */
    run(m, j, 400);
    expect("legacy joiner: refused by policed master", j.state() != IF_STATE_OK);
  }

  /* 5. no id means no upper-layer traffic - the whole point */
  {
    qA.clear(); qB.clear(); rngCtr = 1; useWrongOnJoiner = true;
    Node m("M", 0, &qA, &qB), j("J", 63, &qB, &qA);
    m.ifSetAuthPolicy(ULS_AUTH_LEVEL_OPERATOR, ULS_AUTH_LEVEL_OPERATOR, secretOk);
    j.ifSetAuthPolicy(ULS_AUTH_LEVEL_OPERATOR, ULS_AUTH_LEVEL_OPERATOR, secretOk);
    run(m, j, 400);
    joinerTurn = true; g_uid0 = 0xB0000002;
    j.ifTxPacket->cmd = 0; j.ifTxLen = 4;
    bool blocked = (j.ifSend() == IO_ERROR);
    expect("unauthorized: ifSend refuses upper layer", blocked);
    useWrongOnJoiner = false;
  }

  /* 6. policy with no secret provider is refused outright */
  {
    Node m("M", 0, &qA, &qB);
    expect("policy without provider rejected",
           m.ifSetAuthPolicy(ULS_AUTH_LEVEL_OPERATOR, ULS_AUTH_LEVEL_OPERATOR,
                             nullptr) == IO_ERROR);
  }

  printf("\n%s\n", fails ? "FAILURES" : "handshake behaves");
  return fails != 0;
}
