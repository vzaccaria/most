import sys
import math
import xml.dom.minidom as minidom
#import os
#import os.path
import getopt
import time

# Files
inpFname_XML = None
outFname_XML = None


def main():

    # Read input variables
    x1 = -1
    x2 = -1
    x3 = -1

    doc2 = minidom.parse(inpFname_XML)
    params = doc2.getElementsByTagName("parameter")
    for param in params:
        name = param.getAttribute("name")
        if name in ['x1', 'x2', 'x3']:
            if name == 'x1':
                x1 = float(param.getAttribute("value"))/10
            elif name == 'x2':
                x2 = float(param.getAttribute("value"))/10
            elif name == 'x3':
                x3 = float(param.getAttribute("value"))/10

    if x1 == -1 or x2 == -1 or x3 == -1:
        error_executing_script("Missing input variables")

    sum_xi = (math.pow(x1 - 0.5, 2) - math.cos(20 * math.pi * (x1 - 0.5)))
    sum_xi = sum_xi + (math.pow(x2 - 0.5, 2) -
                       math.cos(20 * math.pi * (x2 - 0.5)))
    sum_xi = sum_xi + (math.pow(x3 - 0.5, 2) -
                       math.cos(20 * math.pi * (x3 - 0.5)))

    gx = 100 * (3 + sum_xi)

    f1 = 0.5 * x1 * x2 * (1 + gx)
    f2 = 0.5 * x1 * (1 - x2) * (1 + gx)
    f3 = 0.5 * (1 - x1) * (1 + gx)

    doc = minidom.Document()
    root = doc.createElementNS(
        "http://www.multicube.eu/", "simulator_output_interface")
    root.setAttribute("xmlns", "http://www.multicube.eu/")
    root.setAttribute("version", "1.3")
    doc.appendChild(root)

    # Write functions f1, f2 and f3 in XML file to send their value to the M3Explorer
    metricNode = doc.createElement("system_metric")
    metricNode.setAttribute("name", "f1")
    metricNode.setAttribute("value", str(f1))
    root.appendChild(metricNode)

    metricNode = doc.createElement("system_metric")
    metricNode.setAttribute("name", "f2")
    metricNode.setAttribute("value", str(f2))
    root.appendChild(metricNode)

    metricNode = doc.createElement("system_metric")
    metricNode.setAttribute("name", "f3")
    metricNode.setAttribute("value", str(f3))
    root.appendChild(metricNode)

    outputString = doc.toprettyxml()
    doc.unlink()
    f = open(outFname_XML, "w")
    f.write(outputString)
    f.close()


def error_executing_script(error_text):
    print(" "+error_text+" ")

    doc = minidom.Document()
    root = doc.createElementNS(
        "http://www.multicube.eu/", "simulator_output_interface")
    root.setAttribute("xmlns", "http://www.multicube.eu/")
    root.setAttribute("version", "1.3")
    doc.appendChild(root)

    errorNode = doc.createElement("error")
    errorNode.setAttribute("reason", error_text)
    errorNode.setAttribute("kind", "fatal")
    root.appendChild(errorNode)

    outputString = doc.toprettyxml()
    doc.unlink()
    f = open(outFname_XML, "w")
    f.write(outputString)
    f.close()
    sys.exit(1)


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
            inpFname_XML = a
        elif o == "--xml_system_metrics":
            outFname_XML = a
        elif o == "--reference_xsd":
            pass
        else:
            usage(sys.argv[0])

    if(inpFname_XML == None or outFname_XML == None):
        usage(sys.argv[0])

    # Initialization

    main()
    sys.exit(0)
