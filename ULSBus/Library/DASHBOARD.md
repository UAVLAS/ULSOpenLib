# Device dashboards

A device entry in the book can carry a `dashboard`: which standard widgets a
tool shows for the device, and which object data feeds each one. It travels in
the device schema (`Scripts/uls_schema.py`), so a tool built before the device
still draws its page. `uls_schema.py` checks every reference against the
device's objects and fails the build on one it cannot resolve.

How objects and variables are named and hidden in a tool is in
[BOOK.md](BOOK.md).

```json
"dashboard": {
  "rate": 100,
  "sections": [
    { "title": "Navigation", "widgets": [
      { "widget": "realtime", "title": "Attitude",
        "series": ["status.attitude"], "labels": ["Roll", "Pitch", "Yaw"] }
    ]}
  ]
}
```

`rate` is how often, in ms, the tool reads the objects the dashboard uses.
Config objects are read once, not polled.

## References

```
instance.variable[index]#bit*scale
```

- `instance` is the object's name on the device (`status`, `cfg`, ...), not
  its type.
- `[index]` picks one element of an array variable.
- `#bit` reads one bit of an integer variable as 0 or 1.
- `*scale` multiplies the value, e.g. `status.lat*1e-7`.

Text (`char`) variables cannot be referenced. Units come from the variable.

A series is a reference string, or `{"ref": ..., "label": ..., "color": ...}`.
A whole array expands to one series per element. `labels` on the widget names
the expanded series in order; a series without a label is called after its
variable, with the element index for arrays.

## Widgets

Every widget takes `widget`, and optionally `title`, `span` (1 or 2 grid
columns), `height` (px) and `rate` (ms, overrides the dashboard's).

| widget | required | optional | shows |
|---|---|---|---|
| `realtime` | `series` | `window` (samples), `labels`, `min`, `max` | scrolling line chart |
| `bar` | `series` | `labels`, `min`, `max` | one bar per series |
| `cartesian` | `x`, `y` (single values) | `trace` (points), `limit` (± axis), `labels` (axis names) | XY position with trace |
| `waterfall` | `source` (whole array) | `min`, `max`, `history` (rows) | spectrum over time |
| `dptf` | `chips`, `slots` (whole arrays) | `margin`, `payload`, `expected` (whole arrays), `flags`, `active`, `inactive`, `stats`, `guard`, `ppm`, `marks`, `good`, `weak` | a pulse-position receiver's last capture |
| `receiver` | at least one of `gauges`, `dacs`, `yaw`, `channels` | `dacsLabels`, `dacsBalance`, `yawLabels`, `channelLabels`, `bins`, `min`, `max`, `stats` | a receiver's main parameters on one card |
| `map` | `lat`, `lon` (deg, single values) | `alt`, `fix` (0/1), `hAcc`, `vAcc`, `vel` (3-element NED), `numSV`, `heading` | position on a map |
| `compass` | `field` (3-element), `offset`, `scale` (3-element float config variables), or `sources` | `normalize`, `limit` | magnetometer calibration, one tab per source |
| `imu` | `sources` | | each IMU source's acc, gyro, field and attitude, one tab per source |
| `health` | `items` | | one lamp per part of the board |

`dptf` shows one capture of a pulse-position packet, not a history: the chip
magnitude envelope (`chips`), the slot each symbol was decoded from (`slots`),
and what came out (`payload`, beside the `expected` one a device transmitting
into its own receiver knows). The envelope is read as `guard` chips, then a
preamble - whatever is left over, with its expected `marks` drawn - then `ppm`
chips per symbol, of which exactly one should be bright. `margin`, one value
per symbol, is how far the winning slot beat the runner-up: at or above `good`
the decision was decisive, below `weak` it is an erasure. `flags` names the
bits it has set, `stats` is a list of references shown as a readout, and while
`active` reads zero the widget says `inactive` instead of judging the capture.

`receiver` puts what a receiver is doing on one card. `gauges` are dials,
each `{"ref", "label", "min", "max", "weak", "good", "count", "hold"}`: `ref`
is a single value drawn over `min`..`max` (0..1 by default), red below `weak`,
green from `good`, amber between. `count` is a counter, or a list of counters
summed, whose rise means something was received: a lamp lights on each rise,
the gauge shows the rate per second, and once the counters have not moved for
`hold` ms (1000) the value is greyed as stale - a device keeps its last
metric, which says nothing about now. `dacs` is a series list, one bar each,
named by `dacsLabels`. With four of them, `dacsBalance` checks the pairs: the
averages (1+2)/2 and (3+4)/2 should match - the difference within a pair is
the yaw, the pairs together see the same light - so the card reads green
while they are within half of `dacsBalance` of their mean, amber up to it,
and red past it: a receiver working badly or a wrong DACS signal. `yaw` is a
series list in degrees, drawn as needles on one compass (0 up, clockwise;
the first series is the bold needle) and named by `yawLabels`. `channels` is a list of whole arrays, one bar chart
each with numbered columns and the peak picked out, all on one scale (`min`,
`max`, else autoscaled) so the channels compare at a glance; `bins` shows only
the first so many elements, for an object sized for more than the firmware
fills. `channelLabels` names the charts. `stats` is a readout, as in `dptf`.

```json
{ "widget": "receiver", "title": "Receiver", "span": 2,
  "gauges": [
    { "ref": "emsg3Debug.metricQ8*0.00390625", "label": "Sync", "max": 2,
      "weak": 0.45, "good": 0.6,
      "count": ["emsg3Debug.packetsOk", "emsg3Debug.packetsBad"] },
    { "ref": "emsg3Debug.marginQ8*0.00390625", "label": "Packet", "max": 8,
      "weak": 2, "good": 4, "count": "emsg3Debug.packetsOk" } ],
  "dacs": ["status.dacsPower[0]", "status.dacsPower[1]",
           "status.dacsPower[2]", "status.dacsPower[3]"], "dacsBalance": 0.5,
  "yaw": ["status.correctedYaw", "status.mrxYaw", "status.dacsYaw"],
  "yawLabels": ["Corrected", "MRX", "DACS"],
  "channels": ["ch1Debug.beansA", "ch2Debug.beansA"], "bins": 32 }
```

`compass` writes `offset` and `scale` back to the device; saving the config
to flash is a separate step the tool asks to confirm. A device with several
compasses lists them in `sources`, each `{"label", "field", "offset",
"scale", "normalize", "limit"}`, and the widget shows one tab per source;
`normalize` and `limit` on the widget apply to every source that does not set
its own. A source with no `offset` and `scale` - a compass another device
calibrates and sends - is captured and checked but not written.

```json
{ "widget": "compass", "title": "Compass calibration", "normalize": true,
  "sources": [
    { "label": "Internal", "field": "imuInt.mag",
      "offset": "cfg.magCalOffset", "scale": "cfg.magCalScale" },
    { "label": "External secondary", "field": "imuExt2.mag" } ] }
```

`imu` shows one IMU source at a time, chosen by tab: each source is
`{"label", "acc", "gyr", "mag", "att", "status", "window"}`, every sensor a
whole 3-element variable and each optional, drawn as a scrolling chart in that
order. `status`, a whole flags variable, shows its bits beside the tabs. Every
source keeps recording while hidden.
With `normalize: true` the field is scaled to a unit sphere (devices that
report a unitless magnetometer); with `false` only the axes are equalised and
the radius kept (field already in uT). No `limit` means autoscale.

`health` shows which part of a board works. Each item is `{"ref", "label",
"ok", "history"}`: `ref` is a single value that reads `ok` (0 by default)
while the part works, so an error bit (`status.error#8`) needs no `ok` and a
good bit (`status.status#2`) takes `"ok": 1`. `history`, a value that turns
non-zero once the part has failed since the device started, makes a lamp
that is fine now read amber rather than green. Red is failing now.

```json
{ "widget": "health", "title": "Board health", "span": 2,
  "items": [
    { "ref": "status.error#8", "label": "Magnetometer (BMM350)",
      "history": "status.error_history#8" },
    { "ref": "status.status#2", "label": "Compass field", "ok": 1 } ] }
```
