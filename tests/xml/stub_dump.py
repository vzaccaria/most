# Stub simulator for m3explorer; it creates the correct output XML document,
# multiplying some of the given parameters as output metrics.
# In addition, it prints out every given parameter and every metric to be
# given back to the explorer, and it tries the XML Schema validation.
# This script expects three parameters, as a specific:
# --xml_system_configuration=input_filename.xml
# --xml_system_metrics=output_filename.xml
# --reference_xsd=input_filename.xsd
#
# @author Sivieri Alessandro

import xml.dom.minidom as minidom
import os.path
import os
import sys
import getopt

# validation
validation = False
# parameters
partest_1 = 0
partest_2 = 0
partest_3 = 0
partest_1_bool = False
partest_2_bool = False
partest_3_bool = False
# files
outputFilename = None
inputFilename = None
referenceXsd = None


def main():
    # repeating global initialization (otherwise an error appears)
    partest_1_bool = False
    partest_2_bool = False
    partest_3_bool = False
    validation = False
    # validation
    validation = os.system(
        "/bin/bash -c \"xmllint --schema %s %s >& /dev/null\"" % (referenceXsd, inputFilename)) == 0
    # reading input params
    if validation:
        doc2 = minidom.parse(inputFilename)
        params = doc2.getElementsByTagName("parameter")
        for param in params:
            name = param.getAttribute("name")
            # parameter work
            if name == "partest_1":
                partest_1 = int(param.getAttribute("value"))
                partest_1_bool = True
            if name == "partest_2":
                partest_2 = int(param.getAttribute("value"))
                partest_2_bool = True
            if name == "partest_3":
                partest_3 = int(param.getAttribute("value"))
                partest_3_bool = True
            # print everything
            if param.hasAttribute("value"):
                value = param.getAttribute("value")
                print("[simulator stub] parameter %s: %s" % (name, value))
            else:
                print("[simulator stub] permutation/mask %s" % (name))
                for item in param.childNodes:
                    if item.localName == "item":
                        print("[simulator stub] index=%s value=%s" % (item.hasAttribute("index") and item.getAttribute(
                            "index") or item.getAttribute("position"), item.getAttribute("value")))
    # writing output params
    doc = minidom.Document()
    root = doc.createElementNS(
        "http://www.multicube.eu/", "simulator_output_interface")
    root.setAttribute("xmlns", "http://www.multicube.eu/")
    root.setAttribute("version", "1.3")
    doc.appendChild(root)
    if not partest_1_bool or not partest_2_bool or not partest_3_bool or not validation:
        if not validation:
            reason = "validation failed"
        else:
            reason = "missing required parameter"
        # error output
        errorNode = doc.createElement("error")
        errorNode.setAttribute("reason", reason)
        errorNode.setAttribute("kind", "fatal")
        root.appendChild(errorNode)
        print("[simulator stub] fatal error: %s" % reason)
    else:
        # metric1
        node = doc.createElement("system_metric")
        node.setAttribute("name", "metric1")
        node.setAttribute("value", str(partest_1*partest_2))
        root.appendChild(node)
        print("[simulator stub] metric metric1: %d" % (partest_1*partest_2))
        # metric2
        node = doc.createElement("system_metric")
        node.setAttribute("name", "metric2")
        node.setAttribute("value", str(partest_2*partest_3))
        root.appendChild(node)
        print("[simulator stub] metric metric2: %d" % (partest_2*partest_3))
    # writing output in the same directory of input
    outputString = doc.toprettyxml()
    doc.unlink()
    f = open(outputFilename, "w")
    f.write(outputString)
    f.close()


def usage(program):
    print("Usage: "+program+" --xml_system_configuration=input_filename.xml --xml_system_metrics=output_filename.xml --reference_xsd=input_filename.xsd")
    sys.exit(1)


if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], "", [
                                   "xml_system_configuration=", "xml_system_metrics=", "reference_xsd="])
    except getopt.GetoptError as err:
        print(err)
        usage(sys.argv[0])
    for o, a in opts:
        if o == "--xml_system_configuration":
            inputFilename = a
        elif o == "--xml_system_metrics":
            outputFilename = a
        elif o == "--reference_xsd":
            referenceXsd = a
        else:
            usage(sys.argv[0])
    if inputFilename == None or outputFilename == None:
        usage(sys.argv[0])
    main()
    sys.exit(0)
