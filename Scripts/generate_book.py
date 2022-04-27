
import argparse
import json
import os
import generate_book_arm 

parser = argparse.ArgumentParser(description='Generate UAVLAS boundle file')

parser.add_argument('-b', '--book',type=str, help='Devices book')
parser.add_argument('-o', '--output',type=str, help='Output source file output_path/ULSDevices.h')

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

print("Generating Objects book for ARM")

generate_book_arm.generate(objects,devices,args.output)



print("------ Book generated ------")
