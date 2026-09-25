
"""
Device schema blobs - the object book a device carries so a tool built before
it can still decode its objects.

For every device in the book this builds one schema: the device entry, the
object instances it carries and the object types they use, with the wire
layout (offset, element size, count) of every variable spelled out so the tool
slices payloads instead of re-deriving packing rules. The schema is serialised
as canonical JSON, deflated, and put behind a small header:

  offset size
   0     4    magic       'ULSS'
   4     1    format      SCHEMA_FORMAT
   5     1    codec       1 = zlib stream (Qt: prepend rawLen big-endian and
                          hand it to qUncompress)
   6     2    pageSize    bytes per data page
   8     2    pageCount   data pages, not counting the header page
  10     2    devType     device type code, as in the book
  12     4    rawLen      uncompressed schema length
  16     4    zLen        compressed length
  20     4    zCrc32      CRC32 of the compressed bytes
  24     4    schemaHash  CRC32 of the uncompressed schema
  28     4    reserved    0
  (all little-endian)

On the bus the blob is read as objects SCHEMA_PAGE_ID_FIRST + n: page 0 is the
32-byte header alone - enough for a tool to hit its cache by schemaHash - and
page n >= 1 is the n-th pageSize slice of the compressed stream.

Run directly to inspect a generated blob:
  python3 uls_schema.py build/book/schemas/ULSDV_HL_G3_RXM.ulss
"""

import json
import os
import re
import struct
import sys
import zlib

SCHEMA_MAGIC = b"ULSS"
SCHEMA_FORMAT = 1
SCHEMA_CODEC_ZLIB = 1
SCHEMA_HEADER_SIZE = 32
SCHEMA_PAGE_SIZE = 1024
SCHEMA_PAGE_ID_FIRST = 0xFE00
SCHEMA_PAGE_ID_LAST = 0xFEFF

_HEADER_FMT = "<4sBBHHHIIIII"
assert struct.calcsize(_HEADER_FMT) == SCHEMA_HEADER_SIZE

# Sizes as the packed structs in generate_structures.py lay them out.
TYPE_SIZES = {"char": 1, "uint8": 1, "int8": 1, "uint16": 2, "int16": 2,
              "uint32": 4, "int32": 4, "float": 4,
              "flags_uint8": 1, "flags_uint16": 2, "flags_uint32": 4,
              "opt_uint8": 1, "opt_uint16": 2, "opt_uint32": 4}

# Keys that describe wire layout; everything else on a variable is metadata
# and passes through untouched, so new book keys need no generator change.
_LAYOUT_KEYS = ("name", "type", "offset", "size", "count")


def _parse_int(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def _canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"),
                      ensure_ascii=True).encode("ascii")


def _strip_descriptions(item):
    item.pop("description", None)
    return item


# Who a tool should show an object or a variable to. Absent means everyone, so
# an ordinary parameter carries no key at all and costs the schema nothing.
# The values are ordered and each tier includes the ones before it. A tool that
# meets a level it does not know must treat it as the most hidden one, so a
# tier added here can never expose anything in tools already in the field.
LEVELS = ("advanced", "service", "developer")


def _check_level(item, where):
    level = item.get("level")
    if level is not None and level not in LEVELS:
        raise ValueError("%s: unknown level %r, expected one of %s" %
                         (where, level, ", ".join(LEVELS)))


def build_object_type(obj, strip_descriptions=False):
    """One object type with explicit layout and its layoutHash."""
    variables = []
    offset = 0
    _check_level(obj, "Object %s" % obj["name"])
    for var in obj["variables"]:
        _check_level(var, "Object %s variable %s" % (obj["name"], var["name"]))
        if var["type"] not in TYPE_SIZES:
            raise ValueError("Object %s variable %s: unknown type %s" %
                             (obj["name"], var["name"], var["type"]))
        out = dict(var)
        count = out.pop("lenght", 1)
        out["offset"] = offset
        out["size"] = TYPE_SIZES[var["type"]]
        out["count"] = count
        if strip_descriptions:
            _strip_descriptions(out)
        variables.append(out)
        offset += out["size"] * count

    layout = [[v[k] for k in _LAYOUT_KEYS] for v in variables]
    result = {"name": obj["name"],
              "description": obj.get("description", ""),
              "access": obj["access"],
              "size": offset,
              # Only when the book says so: absent is the ordinary case.
              **({"level": obj["level"]} if "level" in obj else {}),
              "layoutHash": zlib.crc32(_canonical(layout)),
              "variables": variables}
    if strip_descriptions:
        _strip_descriptions(result)
    return result


# A device's dashboard (ULSBus/Library/DASHBOARD.md) says which widgets show
# which object data. It travels in the schema as the book spells it; these
# checks make a firmware build fail on a reference the tool could not resolve.

_REF_RE = re.compile(r"^([A-Za-z_]\w*)\.([A-Za-z_]\w*)"
                     r"(?:\[(\d+)\])?(?:#(\d+))?"
                     r"(?:\*([-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?))?$")

_INT_TYPES = ("uint8", "int8", "uint16", "int16", "uint32", "int32",
              "flags_uint8", "flags_uint16", "flags_uint32",
              "opt_uint8", "opt_uint16", "opt_uint32")

_COMMON_WIDGET_KEYS = ("widget", "title", "span", "height", "rate")

# widget: (required keys, optional keys)
DASHBOARD_WIDGETS = {
    "realtime": (("series",), ("window", "labels", "min", "max")),
    "bar": (("series",), ("labels", "min", "max")),
    "cartesian": (("x", "y"), ("trace", "limit", "labels")),
    "waterfall": (("source",), ("min", "max", "history")),
    "dptf": (("chips", "slots"), ("margin", "payload", "expected", "flags",
                                  "active", "inactive", "stats", "guard",
                                  "ppm", "marks", "good", "weak")),
    "map": (("lat", "lon"), ("alt", "fix", "hAcc", "vAcc", "vel", "numSV",
                             "heading")),
    "compass": (("field", "offset", "scale"), ("normalize", "limit")),
}


def _resolve_ref(ref, instances, types, where):
    """(instance, variable, index, bit) for a reference, or ValueError."""
    if not isinstance(ref, str):
        raise ValueError("%s: reference must be a string, got %r" % (where, ref))
    m = _REF_RE.match(ref)
    if not m:
        raise ValueError("%s: bad reference %r" % (where, ref))
    inst_name, var_name, index, bit, _ = m.groups()
    inst = instances.get(inst_name)
    if inst is None:
        raise ValueError("%s: no object %r on the device" % (where, inst_name))
    var = next((v for v in types[inst["object"]]["variables"]
                if v["name"] == var_name), None)
    if var is None:
        raise ValueError("%s: %s has no variable %r" % (where, inst_name,
                                                        var_name))
    if var["type"] == "char":
        raise ValueError("%s: %s is text" % (where, ref))
    index = None if index is None else int(index)
    if index is not None and index >= var["count"]:
        raise ValueError("%s: %s has %d elements" % (where, ref, var["count"]))
    bit = None if bit is None else int(bit)
    if bit is not None and (var["type"] not in _INT_TYPES or
                            bit >= 8 * var["size"]):
        raise ValueError("%s: bit %d of %s" % (where, bit, ref))
    return inst, var, index, bit


def _scalar_ref(ref, instances, types, where):
    inst, var, index, bit = _resolve_ref(ref, instances, types, where)
    if index is None and var["count"] != 1:
        raise ValueError("%s: %s is an array; pick an element" % (where, ref))
    return inst, var


def _array_ref(ref, instances, types, where, key="reference"):
    """A whole array variable: no element, no bit, more than one element."""
    inst, var, index, bit = _resolve_ref(ref, instances, types, where)
    if index is not None or bit is not None or var["count"] < 2:
        raise ValueError("%s: %s must be a whole array" % (where, key))
    return inst, var


def _vector_ref(ref, instances, types, where, count=3):
    inst, var, index, bit = _resolve_ref(ref, instances, types, where)
    if index is not None or bit is not None or var["count"] != count:
        raise ValueError("%s: %s must be a whole %d-element variable" %
                         (where, ref, count))
    return inst, var


def validate_dashboard(dashboard, instances, types, device_name=""):
    """Raises ValueError unless every widget resolves against the device."""
    by_name = {inst["name"]: inst for inst in instances}
    if not isinstance(dashboard, dict) or \
            not isinstance(dashboard.get("sections"), list):
        raise ValueError("%s dashboard: needs a sections list" % device_name)
    for s, section in enumerate(dashboard["sections"]):
        widgets = section.get("widgets")
        if not isinstance(widgets, list):
            raise ValueError("%s dashboard section %d: needs a widgets list" %
                             (device_name, s))
        for w, widget in enumerate(widgets):
            kind = widget.get("widget")
            where = "%s dashboard %s/%s" % (
                device_name, section.get("title", s),
                widget.get("title", "%s %d" % (kind, w)))
            if kind not in DASHBOARD_WIDGETS:
                raise ValueError("%s: unknown widget %r" % (where, kind))
            required, optional = DASHBOARD_WIDGETS[kind]
            for key in required:
                if key not in widget:
                    raise ValueError("%s: missing %r" % (where, key))
            for key in widget:
                if key not in required + optional + _COMMON_WIDGET_KEYS:
                    raise ValueError("%s: unknown key %r" % (where, key))
            _validate_widget(kind, widget, by_name, types, where)


def _validate_widget(kind, widget, instances, types, where):
    if kind in ("realtime", "bar"):
        series = widget["series"]
        if not isinstance(series, list) or not series:
            raise ValueError("%s: series must be a non-empty list" % where)
        for entry in series:
            ref = entry.get("ref") if isinstance(entry, dict) else entry
            _resolve_ref(ref, instances, types, where)
    elif kind == "cartesian":
        _scalar_ref(widget["x"], instances, types, where)
        _scalar_ref(widget["y"], instances, types, where)
    elif kind == "waterfall":
        _array_ref(widget["source"], instances, types, where, "source")
    elif kind == "dptf":
        _validate_dptf(widget, instances, types, where)
    elif kind == "map":
        for key in ("lat", "lon", "alt", "fix", "hAcc", "vAcc", "numSV",
                    "heading"):
            if key in widget:
                _scalar_ref(widget[key], instances, types, where)
        if "vel" in widget:
            _vector_ref(widget["vel"], instances, types, where)
    elif kind == "compass":
        _vector_ref(widget["field"], instances, types, where)
        for key in ("offset", "scale"):
            ref = widget[key]
            if "*" in ref:
                raise ValueError("%s: %s is written back; no scale" %
                                 (where, key))
            inst, var = _vector_ref(ref, instances, types, where)
            if var["type"] != "float" or inst.get("type") != "config":
                raise ValueError("%s: %s must be a float config variable" %
                                 (where, key))


def _validate_dptf(widget, instances, types, where):
    """A pulse-position receiver's capture: envelope, symbols, payload."""
    _, chips = _array_ref(widget["chips"], instances, types, where, "chips")
    _, slots = _array_ref(widget["slots"], instances, types, where, "slots")
    if "margin" in widget:
        _, margin = _array_ref(widget["margin"], instances, types, where,
                               "margin")
        if margin["count"] != slots["count"]:
            raise ValueError("%s: margin must have %d elements, as slots does"
                             % (where, slots["count"]))
    for key in ("payload", "expected"):
        if key in widget:
            _array_ref(widget[key], instances, types, where, key)
    if "flags" in widget:
        _, var, index, bit = _resolve_ref(widget["flags"], instances, types,
                                          where)
        if index is not None or bit is not None or var["count"] != 1 or \
                not var["type"].startswith("flags_"):
            raise ValueError("%s: flags must be a whole flags variable" % where)
    if "active" in widget:
        _scalar_ref(widget["active"], instances, types, where)
    for entry in widget.get("stats", []):
        ref = entry.get("ref") if isinstance(entry, dict) else entry
        _resolve_ref(ref, instances, types, where)
    guard = widget.get("guard", 0)
    ppm = widget.get("ppm", 8)
    for key, value in (("guard", guard), ("ppm", ppm)):
        if not isinstance(value, int) or value < 0:
            raise ValueError("%s: %s must be a whole number" % (where, key))
    if ppm < 2:
        raise ValueError("%s: ppm must be at least 2" % where)
    payload_chips = slots["count"] * ppm
    if chips["count"] < guard + payload_chips:
        raise ValueError("%s: chips holds %d, too few for %d guard and %d "
                         "symbols of %d" % (where, chips["count"], guard,
                                            slots["count"], ppm))
    preamble = chips["count"] - guard - payload_chips
    for mark in widget.get("marks", []):
        if not isinstance(mark, int) or not 0 <= mark < preamble:
            raise ValueError("%s: mark %r is outside the %d preamble chips" %
                             (where, mark, preamble))


def build_device_schema(dev, objects_by_name, strip_descriptions=False):
    instances = []
    types = {}
    for ref in dev["objects"]:
        type_name = ref["object"]
        if type_name not in objects_by_name:
            raise ValueError("Device %s object %s: no object type %s in book" %
                             (dev["name"], ref["name"], type_name))
        _check_level(ref, "Device %s object %s" % (dev["name"], ref["name"]))
        address = _parse_int(ref["address"])
        if SCHEMA_PAGE_ID_FIRST <= address <= SCHEMA_PAGE_ID_LAST:
            raise ValueError("Device %s object %s: address 0x%04X is reserved "
                             "for schema pages" % (dev["name"], ref["name"],
                                                   address))
        inst = dict(ref)
        inst["address"] = address
        instances.append(inst)
        if type_name not in types:
            types[type_name] = build_object_type(objects_by_name[type_name],
                                                 strip_descriptions)

    device = {"name": dev["name"],
              "description": dev.get("description", ""),
              "type": _parse_int(dev["type"])}
    if strip_descriptions:
        _strip_descriptions(device)
    if "dashboard" in dev:
        validate_dashboard(dev["dashboard"], instances, types, dev["name"])
        device["dashboard"] = dev["dashboard"]
    return {"format": SCHEMA_FORMAT,
            "device": device,
            "objects": instances,
            "types": types}


def pack_schema(schema, page_size=SCHEMA_PAGE_SIZE):
    """Header + compressed schema, as stored in flash."""
    raw = _canonical(schema)
    packed = zlib.compress(raw, 9)
    page_count = (len(packed) + page_size - 1) // page_size
    if page_count > SCHEMA_PAGE_ID_LAST - SCHEMA_PAGE_ID_FIRST:
        raise ValueError("Schema for %s needs %d pages" %
                         (schema["device"]["name"], page_count))
    header = struct.pack(_HEADER_FMT, SCHEMA_MAGIC, SCHEMA_FORMAT,
                         SCHEMA_CODEC_ZLIB, page_size, page_count,
                         schema["device"]["type"], len(raw), len(packed),
                         zlib.crc32(packed), zlib.crc32(raw), 0)
    return header + packed


def parse_header(page0):
    if len(page0) < SCHEMA_HEADER_SIZE:
        raise ValueError("Schema header too short: %d bytes" % len(page0))
    (magic, fmt, codec, page_size, page_count, dev_type, raw_len, z_len,
     z_crc, schema_hash, _) = struct.unpack_from(_HEADER_FMT, page0)
    if magic != SCHEMA_MAGIC:
        raise ValueError("Bad schema magic %r" % magic)
    return {"format": fmt, "codec": codec, "pageSize": page_size,
            "pageCount": page_count, "devType": dev_type, "rawLen": raw_len,
            "zLen": z_len, "zCrc32": z_crc, "schemaHash": schema_hash}


def page(blob, index):
    """What the device answers for object SCHEMA_PAGE_ID_FIRST + index."""
    if index == 0:
        return blob[:SCHEMA_HEADER_SIZE]
    header = parse_header(blob)
    if index > header["pageCount"]:
        raise IndexError("Schema page %d of %d" % (index, header["pageCount"]))
    start = SCHEMA_HEADER_SIZE + (index - 1) * header["pageSize"]
    return blob[start:start + header["pageSize"]]


def unpack_pages(pages):
    """Reassemble and verify, the way the tool will: header page, then data."""
    header = parse_header(pages[0])
    if header["format"] != SCHEMA_FORMAT:
        raise ValueError("Unsupported schema format %d" % header["format"])
    if header["codec"] != SCHEMA_CODEC_ZLIB:
        raise ValueError("Unsupported schema codec %d" % header["codec"])
    if len(pages) - 1 != header["pageCount"]:
        raise ValueError("Expected %d data pages, got %d" %
                         (header["pageCount"], len(pages) - 1))
    packed = b"".join(pages[1:])
    if len(packed) != header["zLen"] or zlib.crc32(packed) != header["zCrc32"]:
        raise ValueError("Compressed schema length/CRC mismatch")
    raw = zlib.decompress(packed)
    if len(raw) != header["rawLen"] or zlib.crc32(raw) != header["schemaHash"]:
        raise ValueError("Schema length/hash mismatch")
    return header, json.loads(raw)


def unpack_blob(blob):
    header = parse_header(blob)
    return unpack_pages([page(blob, i)
                         for i in range(header["pageCount"] + 1)])


_ARM_HEADER = """/**
 *  Copyright: 2026 by UAVLAS  <www.uavlas.com>
 *
 * This file is part of UAVLAS project applications.
 *
 * @license LGPL-3.0+ <https://spdx.org/licenses/LGPL-3.0+>
 */

// THIS FILE GENERATED AUTOMATICALLY DO NOT EDIT
//
// Compressed object schemas, one per device type, served as objects
// ULS_SCHEMA_PAGE_ID_FIRST + n (page 0 = header, page n = n-th data page).
// Format: ULSOpenLib/Scripts/uls_schema.py. Each schema's data sits in an
// inline function, so only the one a firmware references is linked in. The
// generated device classes in ULSDevices.h point ULSDBase::pxSchema at their
// own; define ULS_NO_DEVICE_SCHEMA to build a firmware without it.

#ifndef ULSDEVICESCHEMAS_H
#define ULSDEVICESCHEMAS_H

#include <inttypes.h>

#include "ULSBusTypes.h"

"""


def _c_bytes(data, indent="      "):
    lines = []
    for i in range(0, len(data), 16):
        lines.append(indent + ",".join("0x%02X" % b for b in data[i:i + 16]))
    return ",\n".join(lines)


def write_layout_asserts(out, objects, struct_prefix):
    """
    The schema's offsets are computed here, not by the compiler. Emitted into
    ULSDevices.h after the structs so the compiler confirms them against the
    bytes firmware really sends.
    """
    out.write("// Device schema layout must match these structs "
              "(Scripts/uls_schema.py).\n")
    for obj in objects:
        obj_type = build_object_type(obj)
        st = struct_prefix + obj["name"]
        out.write("static_assert(sizeof(%s) == %d, \"%s size\");\n" %
                  (st, obj_type["size"], obj["name"]))
        for var in obj_type["variables"]:
            out.write("static_assert(offsetof(%s, %s) == %d, "
                      "\"%s.%s offset\");\n" %
                      (st, var["name"], var["offset"], obj["name"],
                       var["name"]))
    out.write("\n")


def write_arm_header(path, schemas, blobs):
    with open(path, "w") as out:
        out.write(_ARM_HEADER)
        # The bus code parses the blob; make it refuse a mismatched library.
        out.write("static_assert(ULS_SCHEMA_FORMAT == %d, "
                  "\"schema format vs ULSBusTypes.h\");\n" % SCHEMA_FORMAT)
        out.write("static_assert(ULS_SCHEMA_HEADER_SIZE == %d, "
                  "\"schema header vs ULSBusTypes.h\");\n" % SCHEMA_HEADER_SIZE)
        out.write("static_assert(ULS_SCHEMA_PAGE_ID_FIRST == 0x%04X && "
                  "ULS_SCHEMA_PAGE_ID_LAST == 0x%04X, "
                  "\"schema pages vs ULSBusTypes.h\");\n\n" %
                  (SCHEMA_PAGE_ID_FIRST, SCHEMA_PAGE_ID_LAST))

        for schema, blob in zip(schemas, blobs):
            header = parse_header(blob)
            name = schema["device"]["name"]
            out.write("// %s: schema 0x%08X, %d bytes raw, %d compressed, "
                      "%d data pages\n" %
                      (name, header["schemaHash"], header["rawLen"],
                       header["zLen"], header["pageCount"]))
            out.write("struct ULSSchema_%s {\n" % name)
            out.write("  static uint32_t hash() { return 0x%08Xu; }\n" %
                      header["schemaHash"])
            out.write("  static uint32_t size() { return %du; }\n" % len(blob))
            out.write("  static const uint8_t *data() {\n")
            out.write("    static const uint8_t blob[] = {\n")
            out.write(_c_bytes(blob) + "};\n")
            out.write("    return blob;\n  }\n};\n\n")

        out.write("#endif  // ULSDEVICESCHEMAS_H\n")


def generate(objects, devices, output, target, strip_descriptions=False):
    objects_by_name = {obj["name"]: obj for obj in objects}
    schemas = [build_device_schema(dev, objects_by_name, strip_descriptions)
               for dev in devices]
    blobs = [pack_schema(schema) for schema in schemas]

    schema_dir = os.path.join(output, "schemas")
    os.makedirs(schema_dir, exist_ok=True)
    print("Schemas:")
    for schema, blob in zip(schemas, blobs):
        # Round-trip every blob through the reader before anything ships it.
        _, decoded = unpack_blob(blob)
        if decoded != schema:
            raise ValueError("Schema round trip failed for " +
                             schema["device"]["name"])
        header = parse_header(blob)
        name = schema["device"]["name"]
        print(" - %s: 0x%08X raw %d zlib %d pages %d" %
              (name, header["schemaHash"], header["rawLen"], header["zLen"],
               header["pageCount"]))
        with open(os.path.join(schema_dir, name + ".ulss"), "wb") as out:
            out.write(blob)

    if target == "ARM":
        write_arm_header(os.path.join(output, "ULSDeviceSchemas.h"),
                         schemas, blobs)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: uls_schema.py <device>.ulss")
        sys.exit(2)
    with open(sys.argv[1], "rb") as handle:
        header, schema = unpack_blob(handle.read())
    print(json.dumps(header, indent=2))
    print(json.dumps(schema, indent=2, sort_keys=True))
