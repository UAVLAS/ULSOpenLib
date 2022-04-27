
import json

datatypes = {"char": "char", "uint8": "uint8_t","uint16": "uint16_t","uint32": "uint32_t","float":"float",
                "int8": "int8_t","int16": "int16_t","int32": "int32_t",
                "flags_uint8": "uint8_t","flags_uint16": "uint16_t","flags_uint32": "uint32_t"}

_struct_header = "typedef __ULS_PACKET( struct {\n"

_file_header = "/** \n\
 *  Copyright: 2022 by UAVLAS  <www.uavlas.com> \n\
 *  Author: Yury Kapacheuski <yk@uavlas.com> \n\
 *\n\
 * This file is part of UAVLAS project applications.\n\
 *\n\
 * This is free software: you can redistribute\n\
 * it and/or modify it under the terms of the GNU Lesser General Public License\n\
 * as published by the Free Software Foundation, either\n\
 * version 3 of the License, or (at your option) any later version.\n\
 *\n\
 * Some open source application is distributed in the hope that it will\n\
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty\n\
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
 * GNU Lesser General Public License for more details.\n\
 *\n\
 * You should have received a copy of the GNU Lesser General Public License\n\
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.\n\
 *\n\
 * @license LGPL-3.0+ <https://spdx.org/licenses/LGPL-3.0+>\n\
 */\n\
 \n\
 // THIS FILE GENERATED AUTOMATICALY DO NOT EDIT\n\n\
 #include \"ULSObject.h\"\n\n"

_file_footer = "#endif  // ULSDEVICE_ULSQX_H \n"

# def getArmType(type):
#     if type == "char": return "char"
#     if type == "uint8": return "uint8_t"
#     if type == "uint16": return "uin16_t"
#     if type == "uint32": return "uint32_t"
#     if type == "int8": return "int8_t"
#     if type == "int16": return "in16_t"
#     if type == "int32": return "int32_t"
#     if type == "flags_uint8": return "uint8_t"
#     if type == "flags_uint16": return "uin16_t"
#     if type == "flags_uint32": return "uint32_t"
#     if type == "float": return "float"
#     if type == "double": return "double"
# def isTypeFlag(type):
#     return "flags_" in type    


def generate(objects,devices,output):
    



    book_file = open(output + "/ULSDevicesARM.h", "w")
    book_file.write(_file_header)
    
    print("Devices:")
    for dev in devices:
        book_file.write("#define __ULS_DEVICE_TYPE_" + dev["name"] + " (" + dev["type"] + ")\n")

    book_file.write("\n")
    print("Objects:")
    for obj in objects:
        book_file.write("//" + obj["description"] + "\n")
        book_file.write(_struct_header)
        
        for var in obj["variables"]:
            var_str = "    " + datatypes[var["type"]] + " " + var["name"]
            if "lenght" in var: 
                 var_str += "["+ str(var["lenght"]) + "]"     
            var_str += "; // " + var["description"] + "\n"
            book_file.write(var_str)
        
        book_file.write("}) __" + obj["name"] + ";\n\n")
        

    for obj in objects:
        book_file.write("//" + obj["description"] + "\n")
        book_file.write(_struct_header)
        
        for var in obj["variables"]:
            var_str = "    " + datatypes[var["type"]] + " " + var["name"]
            if "lenght" in var: 
                 var_str += "["+ str(var["lenght"]) + "]"     
            var_str += "; // " + var["description"] + "\n"
            book_file.write(var_str)
        
        book_file.write("}) __" + obj["name"] + ";\n\n")

    for obj in objects:
        book_file.write("//" + obj["description"] + "\n")
        book_file.write(_struct_header)
        
        for var in obj["variables"]:
            var_str = "    " + datatypes[var["type"]] + " " + var["name"]
            if "lenght" in var: 
                 var_str += "["+ str(var["lenght"]) + "]"     
            var_str += "; // " + var["description"] + "\n"
            book_file.write(var_str)
        
        book_file.write("}) __" + obj["name"] + ";\n\n")


    book_file.write(_file_footer)


     