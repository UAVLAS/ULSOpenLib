
import json

datatypes = {"char": "char", "uint8": "uint8_t","uint16": "uint16_t","uint32": "uint32_t","float":"float",
                "int8": "int8_t","int16": "int16_t","int32": "int32_t",
                "flags_uint8": "uint8_t","flags_uint16": "uint16_t","flags_uint32": "uint32_t"}

_obj_struct_header = "typedef __ULS_PACKET( struct {\n"

_obj_struct_name_prefix = "__ULSObjectStruct_"

def generate(objects,devices,book_file):
    for obj in objects:
        obj_struct = _obj_struct_name_prefix + obj["name"]

        book_file.write("//" + obj["description"] + "\n")
        #struct definition
        book_file.write(_obj_struct_header)
        for var in obj["variables"]:
            var_str = "    " + datatypes[var["type"]] + " " + var["name"]
            if "lenght" in var: 
                 var_str += "["+ str(var["lenght"]) + "]"     
            var_str += "; // " + var["description"] + "\n"
            book_file.write(var_str)
        book_file.write("}) " + obj_struct + ";\n\n")
