"""
Device schema blobs: python3 -m unittest discover -s Scripts -p 'test_*.py'
"""

import copy
import glob
import json
import os
import struct
import unittest
import zlib

import uls_schema

LIBRARY = os.path.join(os.path.dirname(__file__), "..", "ULSBus", "Library")

# The largest object a GETOBJ answer can carry at the deepest route
# (ULSBusConnection.cpp cnObjMaxLen with IF_PACKET_SIZE 1324, hs 15).
MAX_OBJECT_AT_MAX_ROUTE = 1304


def load_library():
    objects, devices = [], []
    for path in sorted(glob.glob(os.path.join(LIBRARY, "*.json"))):
        with open(path) as handle:
            data = json.load(handle)
        objects += data.get("Objects", [])
        devices += data.get("Devices", [])
    return {o["name"]: o for o in objects}, devices


class LibrarySchemas(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.objects, cls.devices = load_library()

    def test_every_device_round_trips_page_by_page(self):
        for dev in self.devices:
            schema = uls_schema.build_device_schema(dev, self.objects)
            blob = uls_schema.pack_schema(schema)
            header = uls_schema.parse_header(blob)
            pages = [uls_schema.page(blob, i)
                     for i in range(header["pageCount"] + 1)]
            self.assertEqual(len(pages[0]), uls_schema.SCHEMA_HEADER_SIZE)
            for p in pages:
                self.assertLessEqual(len(p), MAX_OBJECT_AT_MAX_ROUTE)
                self.assertGreater(len(p), 0)
            _, decoded = uls_schema.unpack_pages(pages)
            self.assertEqual(decoded, schema, dev["name"])
            self.assertEqual(header["devType"], int(dev["type"], 0))

    def test_dashboards_travel_in_the_schema(self):
        with_dashboard = [d for d in self.devices if "dashboard" in d]
        self.assertTrue(with_dashboard)
        for dev in with_dashboard:
            blob = uls_schema.pack_schema(
                uls_schema.build_device_schema(dev, self.objects))
            _, decoded = uls_schema.unpack_blob(blob)
            self.assertEqual(decoded["device"]["dashboard"], dev["dashboard"],
                             dev["name"])

    def test_levels_and_titles_travel_in_the_schema(self):
        for dev in self.devices:
            schema = uls_schema.build_device_schema(dev, self.objects)
            _, decoded = uls_schema.unpack_blob(uls_schema.pack_schema(schema))
            for ref, inst in zip(dev["objects"], decoded["objects"]):
                self.assertEqual(inst.get("title"), ref.get("title"))
                self.assertEqual(inst.get("level"), ref.get("level"))
            for name, t in decoded["types"].items():
                self.assertEqual(t.get("level"), self.objects[name].get("level"))
                for var, src in zip(t["variables"],
                                    self.objects[name]["variables"]):
                    self.assertEqual(var.get("level"), src.get("level"))

    def test_an_ordinary_variable_carries_no_level(self):
        # Absent is the common case and must stay absent: the schema is paged
        # over the bus and flashed into every device.
        plain = [v for _, o in self.objects.items()
                 for v in uls_schema.build_object_type(o)["variables"]
                 if "level" not in v]
        self.assertTrue(plain)

    def test_layout_is_packed_and_matches_type_sizes(self):
        for name, obj in self.objects.items():
            t = uls_schema.build_object_type(obj)
            offset = 0
            for var in t["variables"]:
                self.assertEqual(var["offset"], offset, name + "." + var["name"])
                self.assertEqual(var["size"], uls_schema.TYPE_SIZES[var["type"]])
                offset += var["size"] * var["count"]
                self.assertNotIn("lenght", var)
            self.assertEqual(t["size"], offset)

    def test_qunccompress_framing(self):
        # What the Qt side does: 4-byte big-endian rawLen + the zlib stream.
        dev = self.devices[0]
        blob = uls_schema.pack_schema(
            uls_schema.build_device_schema(dev, self.objects))
        header = uls_schema.parse_header(blob)
        stream = blob[uls_schema.SCHEMA_HEADER_SIZE:]
        framed = struct.pack(">I", header["rawLen"]) + stream
        raw = zlib.decompress(framed[4:])
        self.assertEqual(len(raw), struct.unpack(">I", framed[:4])[0])


class LayoutHash(unittest.TestCase):

    OBJ = {"name": "T_v1", "description": "d", "access": "config",
           "variables": [
               {"name": "a", "type": "uint8", "default": 1, "description": "x"},
               {"name": "b", "type": "float", "lenght": 3, "default": [0, 0, 0],
                "description": "y", "min": 0, "max": 1}]}

    def hash_of(self, obj):
        return uls_schema.build_object_type(obj)["layoutHash"]

    def test_metadata_does_not_change_layout_hash(self):
        changed = copy.deepcopy(self.OBJ)
        changed["description"] = "other"
        changed["variables"][1]["max"] = 5
        changed["variables"][1]["default"] = [1, 1, 1]
        changed["variables"][0]["units"] = "m"
        self.assertEqual(self.hash_of(self.OBJ), self.hash_of(changed))

    def test_layout_changes_change_layout_hash(self):
        base = self.hash_of(self.OBJ)
        retyped = copy.deepcopy(self.OBJ)
        retyped["variables"][0]["type"] = "int8"
        resized = copy.deepcopy(self.OBJ)
        resized["variables"][1]["lenght"] = 4
        renamed = copy.deepcopy(self.OBJ)
        renamed["variables"][0]["name"] = "c"
        appended = copy.deepcopy(self.OBJ)
        appended["variables"].append({"name": "z", "type": "uint16"})
        for variant in (retyped, resized, renamed, appended):
            self.assertNotEqual(base, self.hash_of(variant))

    def test_strip_descriptions(self):
        t = uls_schema.build_object_type(self.OBJ, strip_descriptions=True)
        self.assertNotIn("description", t)
        self.assertTrue(all("description" not in v for v in t["variables"]))
        self.assertEqual(t["layoutHash"], self.hash_of(self.OBJ))


class Rejects(unittest.TestCase):

    OBJ = LayoutHash.OBJ

    def device(self, address="0x0020", obj="T_v1"):
        return {"name": "D", "description": "", "type": "0x0099",
                "objects": [{"name": "cfg", "object": obj, "address": address}]}

    def test_reserved_schema_page_address(self):
        with self.assertRaises(ValueError):
            uls_schema.build_device_schema(self.device("0xFE10"),
                                           {"T_v1": self.OBJ})

    def test_unknown_object_type(self):
        with self.assertRaises(ValueError):
            uls_schema.build_device_schema(self.device(obj="Nope"),
                                           {"T_v1": self.OBJ})

    def test_unknown_variable_type(self):
        bad = copy.deepcopy(self.OBJ)
        bad["variables"][0]["type"] = "double"
        with self.assertRaises(ValueError):
            uls_schema.build_object_type(bad)

    def test_unknown_level_is_refused(self):
        for spot in ("object", "variable", "instance"):
            obj = copy.deepcopy(self.OBJ)
            dev = self.device()
            if spot == "object":
                obj["level"] = "expert"
            elif spot == "variable":
                obj["variables"][0]["level"] = "expert"
            else:
                dev["objects"][0]["level"] = "expert"
            with self.assertRaises(ValueError, msg=spot):
                uls_schema.build_device_schema(dev, {"T_v1": obj})

    def test_known_levels_are_accepted(self):
        for level in uls_schema.LEVELS:
            obj = copy.deepcopy(self.OBJ)
            obj["variables"][0]["level"] = level
            obj["level"] = level
            dev = self.device()
            dev["objects"][0]["level"] = level
            schema = uls_schema.build_device_schema(dev, {"T_v1": obj})
            self.assertEqual(schema["types"]["T_v1"]["level"], level)
            self.assertEqual(
                schema["types"]["T_v1"]["variables"][0]["level"], level)
            self.assertEqual(schema["objects"][0]["level"], level)

    def test_bad_dashboard_is_refused(self):
        dev = self.device()
        dev["objects"][0]["type"] = "config"
        dev["objects"].append({"name": "st", "object": "T_v1",
                               "address": "0x0010"})
        bad = [
            {"widget": "nope", "series": ["cfg.a"]},
            {"widget": "realtime"},
            {"widget": "realtime", "series": ["cfg.a"], "colour": "red"},
            {"widget": "realtime", "series": ["other.a"]},
            {"widget": "realtime", "series": ["cfg.missing"]},
            {"widget": "realtime", "series": ["cfg.b[3]"]},
            {"widget": "realtime", "series": ["cfg.b#1"]},
            {"widget": "realtime", "series": ["cfg.a#8"]},
            {"widget": "realtime", "series": ["cfg.a*"]},
            {"widget": "realtime", "series": []},
            {"widget": "cartesian", "x": "cfg.b", "y": "cfg.a"},
            {"widget": "waterfall", "source": "cfg.a"},
            {"widget": "waterfall", "source": "cfg.b[1]"},
            {"widget": "map", "lat": "cfg.a", "lon": "cfg.a", "vel": "cfg.a"},
            {"widget": "compass", "field": "cfg.b", "offset": "st.b",
             "scale": "cfg.b"},
            {"widget": "compass", "field": "cfg.b", "offset": "cfg.b*2",
             "scale": "cfg.b"},
        ]
        for widget in bad:
            dev["dashboard"] = {"sections": [{"widgets": [widget]}]}
            with self.assertRaises(ValueError, msg=repr(widget)):
                uls_schema.build_device_schema(dev, {"T_v1": self.OBJ})
        dev["dashboard"] = {"sections": [{"widgets": [
            {"widget": "realtime", "series": ["cfg.b", {"ref": "cfg.a#7"}]},
            {"widget": "cartesian", "x": "cfg.b[0]*1e-3", "y": "cfg.a"},
            {"widget": "waterfall", "source": "st.b"},
            {"widget": "compass", "field": "st.b", "offset": "cfg.b",
             "scale": "cfg.b"},
        ]}]}
        schema = uls_schema.build_device_schema(dev, {"T_v1": self.OBJ})
        self.assertEqual(schema["device"]["dashboard"], dev["dashboard"])

    def test_corrupt_page(self):
        blob = bytearray(uls_schema.pack_schema(
            uls_schema.build_device_schema(self.device(), {"T_v1": self.OBJ})))
        blob[-1] ^= 0xFF
        with self.assertRaises(ValueError):
            uls_schema.unpack_blob(bytes(blob))


if __name__ == "__main__":
    unittest.main()
