
import json
import generate_structures

datatypes = {"char": "char", "uint8": "uint8_t","uint16": "uint16_t","uint32": "uint32_t","float":"float",
                "int8": "int8_t","int16": "int16_t","int32": "int32_t",
                "flags_uint8": "uint8_t","flags_uint16": "uint16_t","flags_uint32": "uint32_t"}

access_types = {"read":"ULSBUS_OBJECT_PERMITION_READONLY",
                "write":"ULSBUS_OBJECT_PERMITION_WRITEONLY",
                "read-write":"ULSBUS_OBJECT_PERMITION_READWRITE",
                "protected":"ULSBUS_OBJECT_PERMITION_PROTECTED",
                "config":"ULSBUS_OBJECT_PERMITION_READWRITE",
                "system":"ULSBUS_OBJECT_PERMITION_SYSCONFIG",
                "admin": "ULSBUS_OBJECT_PERMITION_ADMIN"
                }                


_obj_class_name_prefix = "ULSObject"

_dev_class_name_prefix = "ULSD_"

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
#ifndef ULSDEVICE_ULSQX_H\n\
#define ULSDEVICE_ULSQX_H\n\n\n\
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
    book_file.write('\n')
    for dev in devices:
        book_file.write("#define __ULS_DEVICE_TYPE_" + dev["name"] + "_NAME \"" + dev["name"] + "\"\n")
        

    book_file.write("\n")

    generate_structures.generate(objects,devices,book_file)
    print("Objects:")
    for obj in objects:
        obj_struct = generate_structures._obj_struct_name_prefix + obj["name"]
        obj_class = _obj_class_name_prefix + obj["name"]
       # book_file.write("//" + obj["description"] + "\n")
        #struct definition
      #  book_file.write(_obj_struct_header)
        # for var in obj["variables"]:
        #     var_str = "    " + datatypes[var["type"]] + " " + var["name"]
        #     if "lenght" in var: 
        #          var_str += "["+ str(var["lenght"]) + "]"     
        #     var_str += "; // " + var["description"] + "\n"
        #     book_file.write(var_str)
        # book_file.write("}) " + obj_struct + ";\n\n")
        #class definition
        book_file.write("class " + obj_class + " : public ULSObjectBase {\n public:\n")
        book_file.write(" " + obj_class + "(uint16_t id)\n")
        book_file.write("    " + ": ULSObjectBase(id,\"" + obj["name"] + "\",\"" + obj["description"] + "\"," + access_types[obj["access"]] + "){\n")
        book_file.write("   size = sizeof(" + obj_struct + ");\n")
        book_file.write("   len = 1;\n")
        book_file.write("   _pxData = (uint8_t *)&var;\n  }\n")
        # enums
        for var in obj["variables"]:
           if "flags" in var:

              flags_str ="    typedef enum{\n"
              index = 1;
              for flag in var["flags"]:
                  flags_str += "       " + var["name"] + "_" + flag + " = " + str(index) + ",\n"
                  index <<= 1
              flags_str = flags_str[:-2]
              flags_str += "\n    }" + obj["name"] + "_" + var["name"] + "_flags;\n\n"
              book_file.write(flags_str)
        if obj["access"] == "config":
            book_file.write("  void defaultConfig() override {\n")
            for var in obj["variables"]:
                if "lenght" in var:
                    def_len = len(var["default"])
                    if def_len > var["lenght"]: def_len = var["lenght"]
                    str_lenght = str(def_len)
                    book_file.write("   " + datatypes[var["type"]] + " " + var["name"] + "_def[" + str_lenght + "] = {")
                    str_def_val = ""
                    if var["type"] == "char":    
                        for defvar in var["default"]:
                            str_def_val += "'" + defvar + "',"
                    else:
                        for defvar in var["default"]:
                            str_def_val +=str(defvar) + ","
                    str_def_val = str_def_val[:-1] 
                    book_file.write(str_def_val + "};\n")
                    book_file.write("   for(int i=0;i<"+ str_lenght +";i++)var." + var["name"] + "[i] = " + var["name"] + "_def[i];\n")    
                else :
                    book_file.write("   var." + var["name"] + " = " + str(var["default"]) + ";\n")
            book_file.write("  };\n")
            
            book_file.write("  void validateConfig() override {\n")
            for var in obj["variables"]:
                if ("max" in var) and ("min" in var):
                    if "lenght" in var:
                        book_file.write("   for(int i=0;i<"+ str(var["lenght"]) +";i++)var." + var["name"] + "[i] = " + "checkConfigF(var." + var["name"]+ "[i]," + str(var["min"]) + "," + str(var["max"]) + ");\n")
                    else :
                        book_file.write("   var." + var["name"] + " = " + "checkConfigF(var." + var["name"]+ "," + str(var["min"]) + "," + str(var["max"]) + ");\n")
            book_file.write("  };\n")
        book_file.write(" " + obj_struct + " var;\n};\n" + "// End of " + obj_class + "\n\n")

    # standart base class



    for dev in devices:
        dev_class = _dev_class_name_prefix + dev["name"]
        base_class = s_dev_class_name_prefix + "ULSX"
        book_file.write("//" + dev["name"] +": " + dev["description"] +"\n")
        #class definition
        book_file.write("class " + dev_class + " : public " + base_class + " {\n public:\n")
        book_file.write(" " + dev_class + "():\n")
        str_obj_init = "    " + base_class + "(__ULS_DEVICE_TYPE_" + dev["name"] + "_NAME,__ULS_DEVICE_TYPE_"+ dev["name"] + "),\n"
        
        #class constructor
        for obj in dev["objects"]: 
            str_obj_init += "    o_" + obj["name"] + "("+ obj["address"] + "),\n"
        
        str_obj_init = str_obj_init[:-2]
        book_file.write(str_obj_init + "{\n")    

        for obj in dev["objects"]: 
            book_file.write("  add(&o_" + obj["name"] + ");\n")
         

        for obj_ref in dev["objects"]: 
            for obj in objects:
                if (obj["name"] == obj_ref["object"]) and (obj["access"] == "config"): 
                    book_file.write("  pxCfg = o_" + obj_ref["name"] + "._pxData;\n")
                    book_file.write("  lenCfg = o_" + obj_ref["name"] + ".size;\n")
        
        book_file.write("  };\n") 
        for obj_ref in dev["objects"]: 
            book_file.write("  " + _obj_class_name_prefix + obj_ref["object"] +" o_" + obj_ref["name"] + ";\n")
           
        book_file.write("};\n") 
        book_file.write("// End of " + dev_class + "\n\n")

    book_file.write(_file_footer)


     
