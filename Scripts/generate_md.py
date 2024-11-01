
import json
import generate_structures

access_types = {"read":"ULSBUS_OBJECT_PERMITION_READONLY",
                "write":"ULSBUS_OBJECT_PERMITION_WRITEONLY",
                "read-write":"ULSBUS_OBJECT_PERMITION_READWRITE",
                "protected":"ULSBUS_OBJECT_PERMITION_PROTECTED",
                "config":"ULSBUS_OBJECT_PERMITION_READWRITE",
                "system":"ULSBUS_OBJECT_PERMITION_SYSCONFIG",
                "admin": "ULSBUS_OBJECT_PERMITION_ADMIN"
                }



title = "UAVLAS Devices Book\n"
goon_top = "\n[Go on top](#" + title.replace(" ","-").lower() + ")\n"

def generate(objects,devices,output,log_file):

    # print("Devices:")
    log_file.write("# " + title + "\n")
    log_file.write('## Devices\n')
    log_file.write("|Device name|Type CODE|Description|\n| - | - | - |\n")
    for dev in devices:
        # print(" - Device: " + dev["name"] + " Type: " + dev["type"])
        log_file.write("|[" + dev["name"] + "](#"+ dev["name"].lower() + ") | " + dev["type"] + "|" + dev["description"] + "|\n")

    for dev in devices:
        log_file.write("### " + dev["name"] + "\n **Description:** " + dev["description"] + "\n")
        log_file.write("|Object|Name|Address|Type|\n| - | - | - | - |\n")
        for obj in dev["objects"]:
            if obj.get("type") == None:
                obj["type"] = "generic"
            log_file.write("|[" + obj["object"] + "](#"+ obj["object"].lower() + ")|"+ obj["name"] + "|" + obj["address"] + "|" + obj["type"] +"|\n")
        log_file.write(goon_top)

    # print("\n Objects:")
    log_file.write('## Objects\n')
    for obj in objects:
        # print(" ")
        # print(" - " + obj["name"] + " Description: " + obj["description"])
        log_file.write("### " + obj["name"] + "\n **Description:** " + obj["description"] + "\n")
        # enums bitmasks
        # print("   Variables:")
        log_file.write("#### Variables\n")
        log_file.write("|Name|Type|Lenght|Default|Description|Options and Flags\n| - | - | - | - | - | - |\n")
        for var in obj["variables"]:
           varLen = 1
           optflags = ""
           # print("   - " +  var["name"] + "[" + var["type"] + "] - " + var["description"])
           if "default" not in var:
                var["default"] = "none"
           if "flags" in var:
              index = 1
              optflags += "Flags: "
              for flag in var["flags"]:
                  optflags += flag + " = " + str(index) + "; "
                  index <<= 1
           if "opts" in var:
              index = 0
              optflags += "Options: "
              for opts in var["opts"]:
                  optflags += opts + " = " + str(index) + "; "
                  index += 1
           if "lenght" in var:
               varLen = var["lenght"]
           log_file.write("|" + var["name"] + "|" + var["type"] + "|" + str(varLen) + "|" + str(var["default"]) + "|" + var["description"] + "|" + optflags + '|\n')
        
        log_file.write(goon_top)
              #---------------------------------



