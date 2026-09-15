# Device dashboards

A device entry in the book can carry a `dashboard`: which standard widgets a
tool shows for the device, and which object data feeds each one. It travels in
the device schema (`Scripts/uls_schema.py`), so a tool built before the device
still draws its page. `uls_schema.py` checks every reference against the
device's objects and fails the build on one it cannot resolve.

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
| `map` | `lat`, `lon` (deg, single values) | `alt`, `fix` (0/1), `hAcc`, `vAcc`, `vel` (3-element NED), `numSV`, `heading` | position on a map |
| `compass` | `field` (3-element), `offset`, `scale` (3-element float config variables) | `normalize`, `limit` | magnetometer calibration |

`compass` writes `offset` and `scale` back to the device and saves the config.
With `normalize: true` the field is scaled to a unit sphere (devices that
report a unitless magnetometer); with `false` only the axes are equalised and
the radius kept (field already in uT). No `limit` means autoscale.
