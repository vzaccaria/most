# Stub script for converting XML input values into command line parameters and writing out 
# and XML file.
#
# This script expects two parameters, as a specific:
# --xml_system_configuration=input_filename.xml
# --xml_system_metrics=output_filename.xml
# 
# @author Sivieri Alessandro, Vittorio Zaccaria (2010/2011)
# 
# 1. Converts XML to command line arguments (only scalar supported so far); 
# 2. Runs the use case and greps values from output.
# 3. Writes an output XML file.

import xml.dom.minidom as minidom
import os.path
import os
import sys
import getopt
import re

#parameters
# files
outputFilename=None
inputFilename=None

executable_name='echo @pippo=1030@ @pluto=20@'
patt='@[\w\_]+=[\w\.\_]+@'

def main():
	# reading input params
	bn = os.path.basename(os.getcwd());
	doc2=minidom.parse(inputFilename)
	params=doc2.getElementsByTagName("parameter")

	par_string=""
	for param in params:
		name=param.getAttribute("name")
		par_string=par_string+" --"+name+"="+param.getAttribute("value");

	par_string = par_string +" --base_dir="+bn;
	command = "/usr/bin/time -f \"@wall_time=%e@ @user_time=%U@\" "+executable_name+par_string+" "
	print "Executing "+command
	pin, result, perr = os.popen3(command, "r") 
	rr = result.read()
	ra = perr.read()
	result = rr+ra
	p = re.compile(patt);
	my_result = p.findall(result)

	print my_result

	error=0
	errorcause=None

	mvlist = []
	for mr in my_result:
		p2 = re.compile('[\w\.\_]+');
		mr2 = p2.findall(mr);
		mname = mr2[0];
		mvalue= mr2[1];


		if mname=="error":
			error=1
			errorcause=mvalue

		mvlist.append(mr2)


	#if len(mvlist)==0:
	#	error=1
	#	errorcause="No such a program"
	
	# writing output params
	doc=minidom.Document()
	root=doc.createElementNS("http://www.multicube.eu/", "simulator_output_interface")
	root.setAttribute("xmlns", "http://www.multicube.eu/")
	root.setAttribute("version", "1.3")
	doc.appendChild(root)

	if error:
#		# error output
		errorNode=doc.createElement("error")
		errorNode.setAttribute("reason", errorcause)
		errorNode.setAttribute("kind", "non-fatal")
		root.appendChild(errorNode)
	else:
		for mval in mvlist:
			node=doc.createElement("system_metric")
			node.setAttribute("name", mval[0])
			node.setAttribute("value", mval[1])
			root.appendChild(node)
# writing output in the same directory of input
	outputString=doc.toprettyxml()
	doc.unlink()
	f=open(outputFilename, "w")
	f.write(outputString)
	f.close()

def usage(program):
	print "Usage: "+program+" --xml_system_configuration=input_filename.xml --xml_system_metrics=output_filename.xml --reference_xsd=input_filename.xsd"
	sys.exit(1)

if __name__=="__main__":
	try:
		opts, args=getopt.getopt(sys.argv[1:], "", ["program=", "xml_system_configuration=", "xml_system_metrics=", "reference_xsd="])
	except getopt.GetoptError, err:
		print err
		usage(sys.argv[0])
	for o, a in opts:
		if o == "--xml_system_configuration":
			inputFilename=a
		elif o == "--xml_system_metrics":
			outputFilename=a
		elif o == "--program":
			executable_name=a
		elif o == "--reference_xsd":
			pass
		else:
			usage(sys.argv[0])
	if(inputFilename==None or outputFilename==None):
	   usage(sys.argv[0])
	  
	main()
	sys.exit(0)
