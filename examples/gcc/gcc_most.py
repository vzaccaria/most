# Stub simulator for m3explorer; it creates the correct output XML document,
# multiplying some of the given parameters as output metrics.
# This script expects two parameters, as a specific:
# --xml_system_configuration=input_filename.xml
# --xml_system_metrics=output_filename.xml
#
# @author Sivieri Alessandro

import xml.dom.minidom as minidom
import os.path
import os
import time
import sys
import getopt

# parameters
par_opt = 0
par_dbg = 0
par_opt_bool = False
par_dbg_bool = False
# files
outputFilename = None
inputFilename = None


def main():
    # reading input params
    doc2 = minidom.parse(inputFilename)
    params = doc2.getElementsByTagName("parameter")
    for param in params:
        name = param.getAttribute("name")
        if name == "par_opt":
            par_opt = param.getAttribute("value")
            par_opt_bool = True
        if name == "par_dbg":
            par_dbg = param.getAttribute("value")
            par_dbg_bool = True
    # writing output params
    doc = minidom.Document()
    root = doc.createElementNS(
        "http://www.multicube.eu/", "simulator_output_interface")
    root.setAttribute("xmlns", "http://www.multicube.eu/")
    root.setAttribute("version", "1.3")
    doc.appendChild(root)
    base_dir_executable = os.path.dirname(sys.argv[0])
    base_dir_intermediate_files = os.getcwd()
    if not par_opt_bool or not par_dbg_bool:
        # error output
        errorNode = doc.createElement("error")
        errorNode.setAttribute("reason", "missing required parameter")
        errorNode.setAttribute("kind", "fatal")
        root.appendChild(errorNode)
    else:
        # metric1
        node = doc.createElement("system_metric")
        node.setAttribute("name", "compilation_time")
        t0 = time.time()
        command = "gcc "+ par_opt + " " + par_dbg + " " + base_dir_executable + \
            "/" + targetFilename + " -o " + \
            base_dir_intermediate_files + "/test.out"
        print("--> " + command + " ")
        os.system(command)
        t1 = time.time()-t0
        node.setAttribute("value", str(t1))
        root.appendChild(node)
        # metric2
        node = doc.createElement("system_metric")
        node.setAttribute("name", "code_size")
        executable_file = base_dir_intermediate_files + "/test.out"
        code_size = os.path.getsize(executable_file)
        node.setAttribute("value", str(code_size))
        root.appendChild(node)
    # metric3
        node = doc.createElement("system_metric")
        node.setAttribute("name", "execution_time")
        t0 = time.time()
        os.system(executable_file)
        t1 = time.time()-t0
        node.setAttribute("value", str(t1))
        root.appendChild(node)

# writing output in the same directory of input
    outputString = doc.toprettyxml()
    doc.unlink()
    f = open(outputFilename, "w")
    f.write(outputString)
    f.close()
#	time.sleep(1.5)


def usage(program):
    print("Usage: "+program+" --file=target_filename.c --xml_system_configuration=input_filename.xml --xml_system_metrics=output_filename.xml --reference_xsd=input_filename.xsd")
    sys.exit(1)


if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], "", [
                                   "xml_system_configuration=", "xml_system_metrics=", "reference_xsd=", "file="])
    except getopt.GetoptError as err:
        print(err)
        usage(sys.argv[0])
    for o, a in opts:
        if o == "--xml_system_configuration":
            inputFilename = a
        elif o == "--xml_system_metrics":
            outputFilename = a
        elif o == "--file":
            targetFilename = a
        elif o == "--reference_xsd":
            pass
        else:
            usage(sys.argv[0])
    if(inputFilename == None or outputFilename == None or targetFilename == None):
        usage(sys.argv[0])
    main()
    sys.exit(0)
