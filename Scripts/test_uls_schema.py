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

    def test_corrupt_page(self):
        blob = bytearray(uls_schema.pack_schema(
            uls_schema.build_device_schema(self.device(), {"T_v1": self.OBJ})))
        blob[-1] ^= 0xFF
        with self.assertRaises(ValueError):
            uls_schema.unpack_blob(bytes(blob))


if __name__ == "__main__":
    unittest.main()
