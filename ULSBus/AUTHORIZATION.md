# Authorization on the ULS bus

Admission control at the **interface layer** — `ULSBusInterface`, below routing —
so that a device which has not proved itself never joins the local bus segment
at all, and therefore never gets to send anything the rest of the network would
act on.

**Nothing here is implemented.** This is the protocol and the integration
points, for review before any of it lands.

> An earlier draft of this file put authorization at the connection layer, as a
> routed challenge tied to a session, with explorer left open on every
> interface. That was wrong twice over, and this document replaces it. See
> "What changed" at the end.

## Why layer 1, and why explorer is not an exception

The obvious objection to gating explorer is that a device which cannot be
discovered cannot be routed to, and therefore cannot be authenticated against.
That objection only holds if discovery has to happen *over the ULS bus*. On a
radio it does not: BLE scanning already yields the device type and serial before
anything connects, so the transport does the discovering and the bus never needs
to answer a stranger.

And leaving explorer open is worse than merely untidy. `cnProcessExplorer` calls
`_connections->cnForwardExplorer(this)`, which floods every other connection on
the node, up to 15 hops. **One explorer packet injected on the radio becomes a
flood across the whole internal wired bus** — an amplifier that costs the
attacker one frame and costs the network a traversal, repeatable as fast as the
radio allows. An open explorer is not a small concession; it is the cheapest
attack on the system.

So the boundary is the link, and the rule is simple:

> **The radio link is the trust boundary. Inside it, the internal bus is
> trusted. A peer that has not been admitted on a link gets nothing on that
> link — not objects, not sys, not explorer.**

## The hook already exists

`ULSBusInterface` has a state machine whose semantics are already exactly right.
Interface commands and upper-layer packets are separated by bit `0x10` of `cmd`:
`ifSend()` sets it (`ULSBusInterface.cpp:125`), the NM commands do not, and:

| | |
|---|---|
| `ULSBusInterface.cpp:136` | `send()` refuses upper-layer packets unless `_state == IF_STATE_OK` |
| `ULSBusInterface.cpp:176` | `receive()` discards upper-layer packets unless `_state == IF_STATE_OK` |
| `ULSBusInterface.cpp:171` | layer-1 NM packets are handled by `processLocal()` and never surface |

Everything the connection layer sends and receives — explorer, `GETOBJ`,
`SETOBJ`, `SYS` — rides `cmd | 0x10`. So a device that never reaches
`IF_STATE_OK` is already mute and deaf above layer 1, in both directions,
**with no change to `ULSBusConnection` whatsoever.**

Authorization is therefore one new state on an existing path:

```
IF_STATE_UNINITIALIZED  --REQUEST_ID-->  IF_STATE_AUTH  --proved-->  IF_STATE_OK
                                              |
                                              +--failed / timeout--> IF_STATE_ERROR
```

Explorer gating, sys gating and object gating all fall out of the two lines
already quoted. That is the whole reason this belongs at layer 1.

## The handshake

The existing join is already a two-message exchange with a value echoed back:

```
joiner  --IF_CMD_NM_REQUEST_ID(key)-->  master (_did == 0)
joiner  <--IF_CMD_NM_SET_ID(new_id, key echoed)--  master
```

`processNM_SETID` accepts only if the echoed key matches (`ULSBusInterface.cpp`,
`if (ifRxPacket->set_id.key != _key) return;`). That is request matching, not
security — but the shape is right, and authorization extends it rather than
replacing it.

On an interface with a policy, the master does **not** allocate an id first:

```
joiner                                            master
  |  IF_CMD_NM_REQUEST_ID (nonceA, level, serial)   |
  |------------------------------------------------>|  no slot allocated yet
  |  IF_CMD_NM_AUTH_CHALLENGE (nonceB, proofM)      |
  |<------------------------------------------------|  proofM = MAC(secret, nonceA…)
  |  IF_CMD_NM_AUTH_RESPONSE (proofJ)               |
  |------------------------------------------------>|  verify, then allocateId()
  |  IF_CMD_NM_SET_ID (new_id, nonceA echoed)       |
  |<------------------------------------------------|
```

```
proofM = CMAC(secret, nonceA || masterSerial || level)   master proves to joiner
proofJ = CMAC(secret, nonceB || joinerSerial || level)   joiner proves to master
```

Three points, each earning its place:

- **Mutual, at no extra round trip.** The master proves itself in the same
  message that carries its challenge. Without this a fake platform collects
  responses from every drone that tries to join it. Since both sides hold the
  same fleet secret, mutual authentication is free — refusing it would be the
  odd choice.
- **No state before proof.** `allocateId()` runs only after `proofJ` verifies,
  so an unauthenticated peer cannot consume the 63 local slots. This is the
  same concern as the explorer flood, one layer down.
- **The secret never crosses the link.** Only nonces and MACs do, which is what
  makes this usable on a radio at all.
### New interface commands

`_if_cmd` values 0-6 were taken and `cmd` is masked with `0x0f`, so 7-15 were
free below the `0x10` upper-layer bit:

```c
  IF_CMD_NM_AUTH_REQUEST = 7,
  IF_CMD_NM_AUTH_CHALLENGE = 8,
  IF_CMD_NM_AUTH_RESPONSE = 9,
  IF_CMD_NM_AUTH_REJECT = 10
```

`IF_CMD_NM_REQUEST_ID` is left exactly as it was rather than extended, and a
policed master simply refuses it - it is the unauthenticated join and it proves
nothing. A separate command beat a longer `REQUEST_ID` because it cannot be
misparsed by an old node under any circumstances: an unknown `cmd` falls
through `processLocal`'s switch and is dropped.

Payloads, added to the `_if_packet` union:

```c
struct { uint32_t key; uint32_t uid; uint8_t level; uint8_t nonce[16]; } auth_request;
struct { uint32_t uid; uint8_t level; uint8_t nonce[16]; uint8_t proof[16]; } auth_challenge;
struct { uint8_t proof[16]; } auth_response;
struct { uint8_t reason; } auth_reject;
```

`auth_request.key` is the same request tag `REQUEST_ID` uses, echoed in the
final `SET_ID`, so `processNM_SETID` matches the grant to the request exactly
as it already did.

`IF_CMD_NM_AUTH_REJECT` exists so a refused peer is told. Without it the only
signal is a timeout, and "wrong password" is indistinguishable from "device not
there" - the same defect the object layer still has.

## Per-interface policy

What the question was about. A property of `ULSBusInterface`, not of the
connection:

```c
typedef struct {
  uint8_t required;    /* _uls_auth_level to join; 0 = open interface */
  uint8_t grantLevel;  /* what a successful join is granted */
} _if_auth_policy;

_io_op_result ifSetAuthPolicy(uint8_t required, uint8_t grantLevel,
                              _uls_if_secret_callback secret);
uint8_t ifAuthLevel();
bool ifAuthRequired();
```

The timeout, attempt limit and lockout are compile-time constants
(`IF_AUTH_TIMEOUT_MS` 3000, `IF_AUTH_MAX_ATTEMPTS` 5, `IF_AUTH_LOCKOUT_MS`
10000) rather than per-interface fields: nothing wanted them to differ per
link, and bytes saved here are bytes saved on every device in the tree.
`ifSetAuthPolicy` refuses a policy with no secret provider - demanding a proof
with no way to check it is a configuration error, so it fails at setup rather
than at the first join.

The secret is supplied by a callback, always keyed on the **joiner**: the
master looks it up by the peer uid it was sent, the joiner by its own, and the
two agree because the derivation names one device rather than two.

```c
typedef bool (*_uls_if_secret_callback)(uint8_t level, uint32_t joinerUid,
                                        uint8_t key[ULS_AES_KEY_SIZE]);
```

**Default `required = 0`, which is exactly today's behaviour.** Every existing
device and every existing ULS Tools build keeps working, unaware this exists.
Only an interface that opts in demands anything.

| interface | policy | why |
|---|---|---|
| USART4 to the STM32 | open | inside the box |
| USB-C at J3 | open | reaching the connector is the check |
| BLE service link | `OPERATOR` | the boundary |
| BLE peer link, if one is ever added | `PEER` | the boundary |

## Levels, and the dead enum

```c
typedef enum : uint8_t {
  ULS_AUTH_LEVEL_NONE     = 0,
  ULS_AUTH_LEVEL_PEER     = 1,   /* another device of the fleet */
  ULS_AUTH_LEVEL_OPERATOR = 2,   /* a PC holding the operator secret */
  ULS_AUTH_LEVEL_ADMIN    = 3    /* firmware, factory, sysconfig */
} _uls_auth_level;
```

Admission is binary, but the level granted is worth keeping, and it costs almost
nothing to use: **`ULSBusConnection` already inherits `ULSBusInterface`**
(`class ULSBusConnection : public ULSListItem, public ULSBusInterface`), so the
level established at layer 1 is a member variable the layer-2 permission checks
can read directly. No plumbing, no session lookup.

That lets `ULSBusConnection.cpp:125` and `:149` stop being hardcoded whitelists:

| object permission | readable from | writable from |
|---|---|---|
| `READONLY` | `NONE` | never |
| `WRITEONLY` | never | `NONE` |
| `READWRITE` / `config` | `NONE` | `NONE` |
| `PROTECTED` | `NONE` | `PEER` |
| `SYSCONFIG` | `OPERATOR` | `OPERATOR` |
| `ADMIN` | `ADMIN` | `ADMIN` |

`PROTECTED`, `SYSCONFIG` and `ADMIN` are declared in `ULSBusTypes.h:16-24` and
implemented nowhere; today an object using any of them is unreachable in *both*
directions, because it matches neither whitelist. This makes them work for the
first time, and lets the BLE fleet-key object move from the `"write"` workaround
to a proper `"admin"` declaration.

The `key` field on `ERASE` / `WRITE` / `SETSIGNATURE` / `SAVECFG` then becomes
belt-and-braces rather than the actual protection — which matters, because it is
not protection now:

> **`__DEVICE_KEY` is public.** It is `UID0 ^ UID1 ^ UID2 ^ UID3`
> (`ULSBusTypes.h:9`), and those four words are `serial[4]` inside
> `__ULSObjectSignature`, which every device registers at object `0x0001` as
> `READONLY` (`ULSObject.h:106-112`). One `GETOBJ 0x0001` and the key that
> guards firmware update is computable. On a wire that is tolerable because the
> wire is the check. This design is what makes it tolerable on a radio too.

## What this deliberately does not do

Layer-1 admission authorizes a **link**, not an end-to-end peer. A PC admitted
on the BLE link into the platform can route *through* the module to the STM32
and to anything behind it, at the level it was granted.

That is the stated intent — "we protect the local radio network from our
internal network" — and it should be written down as a decision rather than
discovered later. Two consequences follow:

- **Anything reachable from an admitted radio peer is reachable.** If a device
  behind the boundary must stay out of reach, layer 1 cannot express it; that
  needs a routed, end-to-end session, which is what the previous draft
  described. The two compose if the need ever arises — layer 1 for admission,
  a routed session for finer reach — but building the second now would be
  speculative.
- **A forwarded packet carries no origin level.** The STM32 receiving a
  forwarded request over USART4 sees an open interface and cannot know the
  request began on an authenticated radio link. Inside is trusted; that is the
  same statement, seen from the other end.

## Credentials: derive them, do not list them

A list of peer addresses and passwords on each device works, and stays as the
fallback, but as the default it does not scale: N platforms and M drones is
N×M secrets to distribute, and every new unit means visiting every other unit.

Derive instead. One fleet key per organisation, provisioned once per device over
a wired interface, and

```
secret(role, device) = CMAC(fleetKey, role || deviceSerial)
```

A drone learns a platform's serial from its beacon and derives that platform's
peer secret on the spot. **No list, no configuration, and adding a unit means
provisioning that unit only.**

- Key on the **ULS serial**, not a Bluetooth address — it is in the signature
  object and in the beacon, it survives a module swap, and it is what an
  operator recognises.
- **Fleet key compromise is fleet-wide.** That is the price. Keep `ADMIN` on a
  *separate* key held only by the factory and CI, never the field laptop, so a
  stolen laptop costs config access and not firmware access; and carry a
  `keyEpoch` so a rekey is a distinguishable state rather than a silent outage.

| who | level | holds |
|---|---|---|
| drone ↔ platform | `PEER` | fleet key, derives per-platform peer secret |
| PC in the field | `OPERATOR` | fleet key |
| factory, CI, firmware release | `ADMIN` | separate admin key |
## Hardening the join path

Three things on the pre-authentication path became load-bearing once an
interface faces a radio. All three are now handled.

**1. State was created before any check.** `receive()` called
`deviceConnected()` and set `_locals[src_lid].timeout` for *every* packet, ahead
of the state machine. On a policed interface that let an unauthenticated peer
fill all 63 local slots by varying `src_lid`. Slot creation now happens only on
an open interface or one already in `IF_STATE_OK`.

**2. The nonce was predictable.** `sendNM_REQUESTID` uses
`_key = __DEVICE_KEY ^ _key_cntr`, where `__DEVICE_KEY` is public per above and
`_key_cntr` increments once per task tick — a fine request tag and a useless
nonce. The authorization path draws from `ULS_CRYPTO_RANDOM` instead, and
**refuses to join if there is no entropy** rather than proceeding with a
guessable value.

**3. Nothing rate-limited a join attempt.** `IF_AUTH_MAX_ATTEMPTS` bad proofs
put the interface into an `IF_AUTH_LOCKOUT_MS` silence, and an unanswered
challenge is dropped after `IF_AUTH_TIMEOUT_MS` rather than holding the slot.

A fourth turned up only when two nodes were run against each other: `task()`
began with `if (_did >= 0x3f) _state = IF_STATE_UNINITIALIZED;`, and
`IF_STATE_AUTH` is precisely the state in which having no id yet is correct —
the id is what the handshake is being run to earn. The line tore down every
correct handshake between the challenge and the `SET_ID` answering it. Nothing
in a single-node build could have shown this; `tests/test_auth.cpp` caught it on
the first run and now guards it.

## The crypto, and where the hardware goes

`utils/ULSCrypto.{h,cpp}` — AES-128 encryption, CMAC-AES128, a constant-time
compare, key derivation, and a seeded PRNG. Two things are replaceable per
platform, through the `ULSBusConfig.h` every device already supplies:

```c
#define ULS_CRYPTO_AES128_ENCRYPT  myHardwareAes
#define ULS_CRYPTO_RANDOM          myHardwareTrng
```

`ULSCrypto.h` pulls `ULSBusConfig.h` in itself (guarded by `__has_include`, so
the host tests still build with no platform at all). That matters: without it
`ULSCrypto.cpp` would compile its own CMAC against the software cipher while
every other translation unit used the hardware one.

**The default entropy source is `ulsCryptoRandomNone`, which returns false.**
A platform that provides neither a TRNG nor a seeded PRNG therefore cannot
authenticate at all, rather than authenticating with predictable nonces. That
is the safe direction to fail in, and it is deliberate.

### What the two targets actually have

**nRF52840** — has a hardware RNG, so `ULS_CRYPTO_RANDOM` points at
`sys_csrand_get`. Note `sys_csrand_get` and not `sys_rand_get`: the latter is a
PRNG that may be seeded from something weak. The board DTS also overrides
`zephyr,entropy` from the SoC default `&cryptocell` to `&rng` and `prj.conf`
sets `CONFIG_ENTROPY_CC3XX=n` — `&rng` is a true hardware RNG with bias
correction and sets `ENTROPY_HAS_DRIVER` just the same, and the two together
saved about 10K of flash and 2K of RAM against pulling nrf_cc3xx in for a
handful of bytes per join.

**STM32G0B1** — **has neither AES nor RNG.** Verified against the CMSIS device
headers in this repository: across the whole G0 line only the crypto parts
(G041, G061, G081, G0C1) define `AES_TypeDef` and `RNG_TypeDef`, and the G0B1
is not one of them. So the software cipher is not a fallback there, it is the
only option, and there is no entropy source at all.

For a G0B1 interface that ever needs a policy, seed `ULSCryptoPrng` from a
device secret and a counter, and

> **PERSIST THE COUNTER AND RESTORE IT AHEAD OF THE LAST VALUE USED.**

Its whole security is that the counter never repeats. Restoring a stale one
replays nonces, and a replayed nonce replays the proof that answered it.
Persist ahead in chunks — save `counter + N`, use at most N draws before saving
again — so a reset costs a jump forward and never a step back. On the EIGC-G3
that store is the F-RAM. Nothing wires this up today because no STM32 interface
is policed; the class is implemented and tested, waiting for the first one that
is.

## Cost, measured

| | |
|---|---|
| AES-128 encrypt + S-box | 720 B |
| CMAC | 306 B |
| the handshake in `ULSBusInterface` | ~800 B |
| **total added code** | **~1.9 KByte of flash** |
| RAM per interface | ~32 B |

The nRF application went from 64576 to 76964 bytes, but only about 1.9K of that
is the code above — the rest is the Zephyr entropy driver and what NCS links
alongside it. It sits at 16.4% of a 460K slot. The STM32 EIGC-G3 is at 25.96%
of 368K. Neither is near anything.

ULS Tools is untouched and unaffected while every interface stays open. When
the first policed interface appears it needs the fleet password, the same
derivation, and to show `AUTH_REJECT` as "this device needs a password" rather
than as a dead link.

## What changed from the previous draft

| previous | now | why |
|---|---|---|
| Authorization at the connection layer, routed, end-to-end | At `ULSBusInterface`, link-local | The gate already exists at layer 1 and catches everything above it |
| Explorer always open | Explorer gated like everything else | An open explorer is a flood amplifier into the internal bus |
| Session table keyed by return route | The interface state *is* the session | One peer per link; no table, no lookup |
| Route mixed into the MAC | Not needed | Nothing is forwarded before admission |
| `signEachPacket` for untrusted relays | Dropped | There are no relays inside the boundary |
| Route-change invalidation problem | Gone | — |
| `CN_NACK` needed for the auth flow | Still worth having, but unrelated | Object-permission refusals are silent today; that is its own defect |

The parts that survive unchanged: challenge-response rather than a transmitted
password, mutual authentication, derived rather than listed credentials, the
level model reviving `PROTECTED`/`SYSCONFIG`/`ADMIN`, and the `__DEVICE_KEY`
finding that motivates all of it.
## Status

**Done, and verified on the host:**

- `utils/ULSCrypto.{h,cpp}` — AES-128, CMAC, constant-time compare, key
  derivation, seeded PRNG, with both hardware hooks. Checked against the
  FIPS-197 example and all four RFC 4493 vectors (`tests/test_crypto.cpp`).
- The `IF_STATE_AUTH` handshake in `ULSBusInterface`, the four new commands,
  the policy, the slot-creation fix, the timeout and the lockout. Checked with
  two interfaces wired back to back (`tests/test_auth.cpp`): a matching key
  admits, a wrong one does not, a legacy joiner is refused by a policed master,
  and an unadmitted interface cannot send upper-layer traffic.
- Both firmware trees build. Every STM32 target and the nRF image.

**Not done:**

- **No device enables a policy.** Every interface in the tree is open, so on
  hardware nothing behaves differently from before. The first policed interface
  will be the BLE service link, which does not exist yet.
- **The layer-2 permission mapping.** `ULSBusConnection.cpp:125` and `:149` are
  still the hardcoded whitelists, so `PROTECTED`, `SYSCONFIG` and `ADMIN`
  remain unreachable. The level is already sitting there for it —
  `ifAuthLevel()` is a member of the same object — but this changes behaviour
  for every device rather than none, so it is a separate step.
- **`CN_NACK`.** Object-permission refusals are still silent.
- **The STM32 PRNG seeding.** `ULSCryptoPrng` is implemented and tested but
  wired to nothing, because no STM32 interface is policed yet.
- **Nothing has run on hardware.**

## Staying off until it is wanted

It is deliberate that this sits implemented and unused. Authorization on a link
you are still bringing up turns every ordinary failure into two possible
failures, and the BLE link it is meant to protect does not exist yet. So the
policy stays open through development, and the whole of it is one call away:
until something calls `ifSetAuthPolicy`, none of this code runs.

The cost of that decision is rot. Nothing on any device exercises the join
path, so it can be broken by an edit made for entirely unrelated reasons and
nothing will say so — which is exactly how `task()` came to tear down its own
handshake. `tests/test_auth.cpp` is the whole mitigation, and it is wired into
the repository's `utest.sh` for that reason. **Run it after touching
`ULSBusInterface`, whatever the reason.**

### When it is turned on

1. **Provision before policing.** A device with a policed interface and no
   fleet key cannot be reached to be given one. Write the key over a wired
   interface first — the USB-C at J3 or USART4, both of which stay open — and
   only then set a policy on the radio.
2. **Seed the entropy.** On the nRF this is already done. On any STM32G0B1
   interface, seed `ULSCryptoPrng` and persist its counter *ahead* of use; see
   "What the two targets actually have" above. An unseeded device refuses to
   join rather than joining weakly, so this failure is loud.
3. **One interface at a time**, starting with the BLE service link. The wired
   interfaces have no reason to ever be policed: reaching the connector is
   already the check.
4. **Then, separately, the layer-2 mapping.** Turning `ifAuthLevel()` into the
   object permission check is what makes `PROTECTED`/`SYSCONFIG`/`ADMIN` real,
   and it is what the settings and options protocols this was built for will
   want. It changes behaviour for every device, so it wants its own change and
   its own testing rather than riding along with the first policed link.
