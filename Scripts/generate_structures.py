
import json

datatypes = {"char": "char", "uint8": "uint8_t","uint16": "uint16_t","uint32": "uint32_t","float":"float",
                "int8": "int8_t","int16": "int16_t","int32": "int32_t",
                "flags_uint8": "uint8_t","flags_uint16": "uint16_t","flags_uint32": "uint32_t",
                "opt_uint8": "uint8_t","opt_uint16": "uint16_t","opt_uint32": "uint32_t"}

_obj_struct_header = "typedef __ULS_PACKET( struct {\n"

_obj_struct_name_prefix = "__ULSObjectStruct_"

def write_device_defaults(book_file, obj_class, obj_ref, objects):
    """The class a device declares an object instance with. An instance whose
    book entry has "defaults" gets a subclass that sets them after the object
    type's own, so one device can start a shared config object differently.
    Returns the class name to declare the member with."""
    defaults = obj_ref.get("defaults")
    if not defaults:
        return obj_class
    obj = next(o for o in objects if o["name"] == obj_ref["object"])
    variables = {v["name"]: v for v in obj["variables"]}
    for name in defaults:
        if name not in variables or "lenght" in variables[name]:
            raise ValueError("Object instance %s defaults: %s is not a scalar "
                             "variable of %s" %
                             (obj_ref["name"], name, obj["name"]))
    cls = obj_class + "_" + obj_ref["name"] + "_defaults"
    book_file.write("  class " + cls + " : public " + obj_class + " {\n")
    book_file.write("   public:\n    using " + obj_class + "::" + obj_class + ";\n")
    book_file.write("    void defaultConfig() override {\n")
    book_file.write("      " + obj_class + "::defaultConfig();\n")
    for name, value in defaults.items():
        book_file.write("      var." + name + " = " + str(value) + ";\n")
    book_file.write("    }\n  };\n")
    return cls

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
