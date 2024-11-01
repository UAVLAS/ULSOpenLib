
import argparse
import json
import os
import generate_book_arm 
import generate_book_qt
import generate_md

parser = argparse.ArgumentParser(description='Generate UAVLAS boundle file')

parser.add_argument('-b', '--book',type=str, help='Devices book')
parser.add_argument('-o', '--output',type=str, help='Output source file output_path/ULSDevices.h')
parser.add_argument('-t', '--target',type=str, help='Target System [ARM,QT]')

args = parser.parse_args()



print("------ UAVLAS DEVICE LIBS CREATOR ------")
print(" (C) Yury Kapacheuski 2021. ")

print("Book: " + args.book)
print("Output dir: " + args.output)

# Opening JSON file
f = open(args.book)
 
# returns JSON object as
# a dictionary
data = json.load(f)

objects = data["Objects"]
devices = data["Devices"]
 
print("Book Version: " + data["BookVersion"])

log_file = open(args.output + "/ULSDevices.md", "w")

generate_md.generate(objects,devices,args.output,log_file)

if args.target == "ARM":
    print("Generating Objects book for ARM")
    generate_book_arm.generate(objects,devices,args.output)

if args.target == "QT":
    print("Generating Objects book for QT")
    generate_book_qt.generate(objects,devices,args.output)

print("------ Book generated ------")
