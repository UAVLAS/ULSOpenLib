# tests

Host tests. No framework and no build system on purpose — they must stay
runnable from a bare checkout with nothing installed.

```sh
g++ -std=c++17 -Wall -Wextra -O2 -I utils \
    -o /tmp/test_crypto tests/test_crypto.cpp utils/ULSCrypto.cpp && /tmp/test_crypto

g++ -std=c++17 -O1 -I tests/hostif -I utils -I ULSBus \
    -o /tmp/test_auth tests/test_auth.cpp ULSBus/ULSBusInterface.cpp \
    utils/ULSCrypto.cpp && /tmp/test_auth

g++ -std=c++17 -O1 -I tests/hostif -I utils -I ULSBus -I ULSSerial \
    -o /tmp/test_objects tests/test_objects.cpp ULSBus/ULSBusConnection.cpp \
    ULSBus/ULSBusInterface.cpp ULSBus/ULSObject.cpp ULSSerial/ULSSerial.cpp \
    utils/ULSCrypto.cpp && /tmp/test_objects
```

| | |
|---|---|
| `test_crypto.cpp` | AES-128 against the FIPS-197 example, CMAC against all four RFC 4493 vectors, plus the PRNG and key-derivation behaviour bus authorization relies on |
| `test_auth.cpp` | Two `ULSBusInterface` instances wired back to back: that a matching fleet key admits, that a wrong one does not, that a legacy unauthenticated joiner is refused by a policed master, and that an unadmitted interface cannot send upper-layer traffic |
| `test_objects.cpp` | Two `ULSBusConnection` instances wired back to back over COBS, with a device carrying real objects: that a GETOBJ comes back with the object's bytes, that a 444-byte object (the largest in the library) survives the round trip intact, that permissions are enforced, that a SETOBJ writes what it was given, and that a burst of requests is answered in full |
| `hostif/` | The two headers a device normally supplies — `ULSBusConfig.h` and `ULSDevices.h` — stubbed just enough to build the interface on a host |

Run `test_crypto` after any change to `utils/ULSCrypto.cpp`. A cipher that is
subtly wrong still produces convincing-looking output, so the vectors are the
only thing standing between a typo and an authorization scheme that appears to
work.

Run `test_auth` after any change to the join path in `ULSBus/ULSBusInterface.cpp`
— including changes that look unrelated to authorization. It already caught one:
`task()` forced `_state` back to `IF_STATE_UNINITIALIZED` whenever the interface
had no id, which is precisely the situation `IF_STATE_AUTH` exists to describe,
so a correct handshake was being torn down between the challenge and the reply.
Nothing in a single-node build would have shown that.

Run `test_objects` after any change to the object paths in
`ULSBus/ULSBusConnection.cpp` or to the framing in `ULSSerial/ULSSerial.cpp`.
It is the only thing that exercises a whole GETOBJ/SETOBJ round trip, bounds
checks included, without a device on the bench.
