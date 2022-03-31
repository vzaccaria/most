# Stub simulator for m3explorer; it creates the correct output XML document, based on a given tag set;
# with the specified probability, it also generates an error output.
# This script expects two parameters, as a specific:
# --xml_system_configuration=input_filename.xml
# --xml_system_metrics=output_filename.xml
#
# @author Sivieri Alessandro

import xml.dom.minidom as minidom
import random
import os.path
import os
import sys
import getopt

# error probability: the random number generated in the range 1-100 will be checked against this value
error = 2
# output tag list: metrics name
metrics = ["metric1"]
# error kinds
kinds = ["fatal", "non-fatal"]
# parameters
outputFilename = None
inputFilename = None


def main():
    doc = minidom.Document()
    root = doc.createElementNS(
        "http://www.multicube.eu/", "simulator_output_interface")
    root.setAttribute("xmlns", "http://www.multicube.eu/")
    root.setAttribute("version", "1.3")
    doc.appendChild(root)
    if random.randint(1, 100) < error:
        # error output
        errorNode = doc.createElement("error")
        errorNode.setAttribute("reason", "probability failure")
        errorNode.setAttribute("kind", random.sample(kinds, 1)[0])
        root.appendChild(errorNode)
    else:
        # normal output
        for metric in metrics:
            node = doc.createElement("system_metric")
            node.setAttribute("name", metric)
            node.setAttribute("value", str(random.randint(1, 1000)))
            root.appendChild(node)
    # writing output in the same directory of input
    outputString = doc.toprettyxml()
    doc.unlink()
    f = open(outputFilename, "w")
    f.write(outputString)
    f.close()


def usage(program):
    print("Usage: "+program +
          " --xml_system_configuration=input_filename.xml --xml_system_metrics=output_filename.xml")
    sys.exit(1)


if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(
            sys.argv[1:], "", ["xml_system_configuration=", "xml_system_metrics="])
    except getopt.GetoptError as err:
        print(err)
        usage(sys.argv[0])
    for o, a in opts:
        if o == "--xml_system_configuration":
            inputFilename = a
        elif o == "--xml_system_metrics":
            outputFilename = a
        else:
            usage(sys.argv[0])
    if(inputFilename == None or outputFilename == None):
        usage(sys.argv[0])
    main()
    sys.exit(0)
