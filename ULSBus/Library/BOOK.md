# How the book is shown

A tool draws a device from its schema alone, so anything about *presentation*
has to travel in the book. Two keys do that: `title` names a tab, and `level`
says who the thing is for. Both are metadata — `uls_schema.py` passes them
through untouched and `layoutHash` ignores them, so adding either to a device
already in the field changes nothing on the wire.

The data a dashboard draws is a separate thing, in [DASHBOARD.md](DASHBOARD.md).

## `title` — what a tab is called

An object instance's `name` is its identifier: dashboards reference it
(`status.rel_pos_ned`), so it cannot be renamed to read better. `title` is the
name a person sees.

```json
{ "name": "cfg", "title": "Config", "object": "ULSDVOBJ_RX_Config_v2",
  "address": "0x0020", "type": "config" }
```

Without a `title` a tool capitalises the name, so `status` needs no key and
`cfg` does. Write one wherever the name is an abbreviation, carries a number,
or reads as code.

## `level` — who it is for

`level` goes on a variable, on an object type, or on one object instance of a
device. It is a display hint that lets a tool keep the everyday page short:

| `level` | who sees it | example |
|---|---|---|
| *(absent)* | everyone | `name`, `rxOffsetF`, `rxYaw` |
| `advanced` | integrators tuning a working install | `relPosGain`, `predictionTime` |
| `service` | UAVLAS service; misuse breaks the unit | `magCalOffset`, `compass1Scale` |
| `developer` | firmware work only | `configCRC`, `fftbuf`, `blitzDebug` |

The levels are ordered and each includes the ones before it: a tool set to
`service` shows everything except `developer`. Rules a tool must follow:

- **Absent means everyone.** An ordinary parameter carries no key, which is
  also why the schema barely grows: the whole library costs about 50 bytes a
  device.
- **An unknown level is the most hidden one.** A tool that meets a level it
  was not built with must treat it as `developer`, so a level added here can
  never expose anything in tools already in the field.
- **An object's level is the stricter of its type's and its instance's.** The
  type sets the floor — `ULSQR1R2FFT` is `developer` wherever it appears — and
  a device can only hide an object further, never expose it. An object type
  that is a debug page on one device and the status page on another therefore
  carries no level of its own; the devices that want it hidden say so:

  ```json
  { "name": "bleDebug", "title": "BLE", "object": "ULSDVOBJ_EIGC_BLE_Debug_v1",
    "address": "0x0034", "level": "service" }
  ```
- **A value that is not its default stays visible.** Hiding a `service` field
  someone has already changed would make the page lie about the device.

`level` is not protection. Nothing stops a tool writing a `developer` field,
and the schema is readable by anyone on the bus; what a device refuses is set
by the object's `access` and, once it is enabled, by
[AUTHORIZATION.md](../AUTHORIZATION.md).
