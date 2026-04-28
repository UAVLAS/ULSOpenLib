
import argparse
import json
import os
import generate_book_arm
import generate_book_qt
import generate_md

parser = argparse.ArgumentParser(description='Generate UAVLAS bundle file')

parser.add_argument('-b', '--book', type=str, help='Devices book')
parser.add_argument('-o', '--output', type=str,
                    help='Output source file output_path/ULSDevices.h')
parser.add_argument('-t', '--target', type=str, help='Target System [ARM,QT]')

args = parser.parse_args()


def _load_book_sources(book_path):
    if os.path.isdir(book_path):
        json_files = []
        for entry in sorted(os.listdir(book_path)):
            file_path = os.path.join(book_path, entry)
            if os.path.isfile(file_path) and entry.lower().endswith(".json"):
                json_files.append(file_path)
        if not json_files:
            raise ValueError(
                "No JSON book files found in directory: " + book_path)
        return json_files

    if os.path.isfile(book_path):
        return [book_path]

    raise ValueError("Book path does not exist: " + book_path)


def _merge_book_data(book_files):
    merged_objects = []
    merged_devices = []
    object_names = set()
    device_names = set()
    book_version = None

    for book_file in book_files:
        with open(book_file) as handle:
            data = json.load(handle)

        if book_version is None:
            book_version = data.get("BookVersion", "unknown")
        elif "BookVersion" in data and data["BookVersion"] != book_version:
            raise ValueError("BookVersion mismatch in file: " + book_file)

        for obj in data.get("Objects", []):
            if obj["name"] in object_names:
                raise ValueError("Duplicate object name: " +
                                 obj["name"] + " in file: " + book_file)
            object_names.add(obj["name"])
            merged_objects.append(obj)

        for dev in data.get("Devices", []):
            if dev["name"] in device_names:
                raise ValueError("Duplicate device name: " +
                                 dev["name"] + " in file: " + book_file)
            device_names.add(dev["name"])
            merged_devices.append(dev)

    return book_version, merged_objects, merged_devices


print("------ UAVLAS DEVICE LIBS CREATOR ------")
print(" (C) Yury Kapacheuski 2026. ")

print("Book: " + args.book)
print("Output dir: " + args.output)

book_files = _load_book_sources(args.book)
print("Book files:")
for book_file in book_files:
    print(" - " + book_file)

book_version, objects, devices = _merge_book_data(book_files)

print("Book Version: " + book_version)

log_file = open(args.output + "/ULSDevices.md", "w")

generate_md.generate(objects, devices, args.output, log_file)

if args.target == "ARM":
    print("Generating Objects book for ARM")
    generate_book_arm.generate(objects, devices, args.output)

if args.target == "QT":
    print("Generating Objects book for QT")
    generate_book_qt.generate(objects, devices, args.output)

print("------ Book generated ------")
