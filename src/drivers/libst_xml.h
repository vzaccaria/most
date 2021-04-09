/* @M3EXPLORER_LICENSE_START@
 *
 * This file is part of the Multicube Explorer tool.
 *
 * Authors: Vittorio Zaccaria, Gianluca Palermo, Giovanni Mariani
 * Copyright (c) 2008, Politecnico di Milano, Universita' della Svizzera italiana
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 
 *  * Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 * Neither the name of Politecnico di Milano nor Universita' della Svizzera Italiana 
 * nor the names of its contributors may be used to endorse or promote products 
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * @M3EXPLORER_LICENSE_END@ */



/*
 * Note: Significant and important parts of this source file have been contributed by Alessandro Sivieri while at
 * Politecnico di Milano (2008).
 */



#ifndef LIBM3_XML_DRIVER_H_
#define LIBM3_XML_DRIVER_H_

#include <cmath>
#include <string>
#include <vector>
#include <st_driver.h>
#include <st_vector.h>
#include <st_map.h>
#include <st_exception.h>
#include <st_design_space.h>
/*
 * libxml is a C library, so we have to do this little hack
 * to make it work here, under C++
*/
extern "C"
{
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlschemas.h>
}

using namespace std;

/**
 * This class defines the exception used in all objects of this
 * driver; it accepts only one parameter, the explanation for the
 * error.
 * 
 * @author Sivieri Alessandro
 */
class st_xml_exception: public st_exception
{
	public:
		st_xml_exception(string);
		~st_xml_exception() throw();
};

/**
 * A rule represents an XML fragment describing some properties that
 * a point to be simulated must follow; this XML is parsed for each
 * validation request.
 * 
 * @author Sivieri Alessandro
 */
class st_xml_rule
{
	public:
		st_xml_rule(xmlNodePtr);
		~st_xml_rule();
		xmlNodePtr get_rule();
		void set_rule(xmlNodePtr);
		bool validate(st_point&) throw(st_xml_exception);
	
	private:
		xmlNodePtr rule;
		/**
		 * This is the recursive function which validates a point against a rule;
		 * we first check the operator type, and then we recurse within the inner
		 * operands; the stack pops when we find parameters and/or constants.
		 * The parameter types are only integer (for every operator) and string
		 * (only for equal and not-equal); the return value is an instance of
		 * st_string, st_integer or st_bool, and needs to be destroyed once
		 * it becomes useless.
		 * We do not have to check every possible subclass of st_object: some
		 * obvious rules (like booleans for an 'and' operator) are checked
		 * by the XML validation.
		 * 
		 * @param node the XML node, containing either an operator or a parameter
		 * @param point the list of data to be checked against the rule
		 * @return a pointer to an st_string/integer/bool
		 */
		st_object *parse(xmlNodePtr& node, st_point& point) throw(st_xml_exception);
};

/**
 * This abstract class represents a reader/writer for an XML
 * document; read/write methods have to be implemented by any
 * subclass. It also offers an helper method for XPath search
 * and the current version used in the documents.
 * 
 * @author Sivieri Alessandro
 */
class st_xml_io
{
	public:
		virtual xmlDocPtr read(string)=0;
		virtual void write(string)=0;
		xmlXPathObjectPtr get_nodeset(xmlDocPtr, xmlChar *);
		static const string current_version;
};

/**
 * This is a design space reader/writer; it reads the input
 * document associated and fills every field of a st_design_space
 * object. It also has getters for the other properties of the
 * simulations to be (like metrics or rules).
 * 
 * @author Sivieri Alessandro
 */
class st_xml_design_space: public st_xml_io
{
	public:
		st_xml_design_space();
		~st_xml_design_space();
		xmlDocPtr read(string) throw(st_xml_exception);
		void write(string) throw(st_xml_exception);
		void fill(st_design_space *) throw(st_xml_exception);
		vector<st_xml_rule *> get_rules();
		string get_simulator();
		xmlDocPtr get_doc();
		void set_doc(xmlDocPtr);
	
	private:
		void add_parameter(st_design_space *, st_env *, xmlDocPtr, xmlNode *);
		vector<st_xml_rule *> rules;
		string simulator;
		xmlDocPtr doc;
};

/**
 * This represents an instance of the simulator; there is no
 * need to create a new object for each simulation launch, just
 * call run() with different maps.
 * The initialization method needs to be called before any invocation,
 * and the shutdown method after the last usage of this class.
 * 
 * @author Sivieri Alessandro
 */
class st_xml_simulator: public st_xml_io
{
	public:
		st_xml_simulator(string);
		~st_xml_simulator();
		xmlDocPtr read(string) throw(st_xml_exception);
		void write(string) throw(st_xml_exception);
		/**
		 * Method for launching each simulation: it creates correct files,
		 * forks and the child process executes the simulator.
		 * When the simulation is done, it returns a pointer to the output
		 * (or error) document; this document has to be freed once it has been
		 * used.
		 * The XML exchange structure is as follows:
		 * a) the driver creates a directory called "m3explorer%p", where %p
		 * is the pid of m3explorer process (unique during a full execution);
		 * b) each simulation execution creates a file like "input%p.%d.%h.xml" in
		 * the directory (a), where %p is the pid of a child process of m3explorer
		 * (guaranteed by the system to be unique for each simulate() launch,
		 * thus permitting parallel executions), %d is the current date as %d-%m-%Y
		 * and %h is the current time as %H-%M-%S;
		 * c) the relative path of (b) is passed as first argument
		 * to the simulator tool, the second argument is the computed output
		 * filename;
		 * d) the simulator uses the input file, and has to create a file
		 * called as its second parameter;
		 * e) the explorer tool loads (d), validate it against its schema,
		 * and returns it to the caller (to be parsed);
		 * g) during driver destruction, temporary directory and its content
		 * are removed.
		 * 
		 * @param parameters parameters to be sent to the simulation tool
		 */
		void run(map<string, string> parameters) throw();
		map<string, double> get_metrics();
		int get_error_code();
		string get_error_description();
		map<string, string> get_parameters();
        string get_relative_path();
		static void init() throw(st_xml_exception);
		static void shutdown();
	
	private:
		xmlDocPtr doc;
		map<string, string> parameters;
		map<string, double> metrics;
		int error_code;
		string error_description;
		string simulator;
		static unsigned int counter;
		static string directory;
        static string relative_directory;
		static bool del_element(const char *);
};

/**
 * This singleton class offers two different XML Schema validators,
 * one for each XML document type used by the driver, and each can
 * validate a document against it.
 * The shutdown method needs to be called after the last usage of
 * this class.
 * 
 * @author Sivieri Alessandro
 */
class st_xml_validator
{
	public:
		bool validate(xmlDocPtr doc);
		static st_xml_validator *get_design_space_validator() throw(st_xml_exception);
		static st_xml_validator *get_simulator_validator() throw(st_xml_exception);
		static void shutdown();
		~st_xml_validator();
		static const string design_space_schema;
        static const string simulator_schema;
        string build_path;
	
	private:
		st_xml_validator(string) throw(st_xml_exception);
		static st_xml_validator *design_space_validator;
		static st_xml_validator *simulator_validator;
		string current_schema;
		xmlSchemaParserCtxtPtr schema_context_run;
		xmlSchemaValidCtxtPtr schema_valid_run;
		xmlSchemaPtr schema_run;
};

/**
 * This is the main driver class: it offers every method declared
 * in the interface, creating and calling each other class previously
 * declared, thus demanding to those part of the elaboration.
 * 
 * @author Sivieri Alessandro
 */
class st_xml_driver: public st_driver
{
	public:
        string get_information();
        /**
         * This method launches a simulation to the simulator tool specified
         * in the input XML file, with the given values; it returns the
         * results of this simulation.
         * The st_point has to be checked for validation before calling
         * this method.
         * This method supports parallel executions.
         * The actual implementation executes all the simulations registered
         * in the input file sequentially, adding all metric results into
         * the output point.
         * 
         * @param point the point to be transmitted to the simulator
         * @param env the environment of execution of the exploration tool
         * @return a copy of the simulated point with added metrics
         */
        st_point *simulate(st_point& point, st_env *env);
        bool is_valid(st_point &, st_env *);
        string get_name();
        st_xml_driver() throw(st_xml_exception);
        ~st_xml_driver();
        st_design_space *get_design_space(st_env *) throw(st_xml_exception);
        string get_point_representation(st_env *, st_point *);
        
	private:
        st_xml_simulator *simulator;
		vector<st_xml_rule *> rules;
		xmlDocPtr launch(string, xmlDocPtr) throw(st_xml_exception);
};

inline bool is_exp2(int n)
{
	if(n<1) return false;
	float exp2=log(n)/log(2);
	int res=(int) pow((float) 2, exp2);
	return n==res;
}

inline int contains(st_vector *v, st_string str)
{
	st_object *obj;
	
	for(int i=0; i<v->size(); ++i)
	{
		*obj=v->get(i);
		if(is_a<st_string *>(obj))
		{
			string str2=to<st_string *>(obj)->get_string();
			if(str2.compare(str.get_string())==0)
			{
				return i;
			}
		}
	}
	return -1;
}

inline bool is_int(string constant)
{
	ostringstream counterexample;
	int cvalue=atoi(constant.c_str());
	counterexample << cvalue;
	if(counterexample.str().compare(constant)==0) // no errors in conversion, constant should be an integer
	{
		return true;
	}
	else // conversion error, constant should be a string
	{
		return false;
	}
}

inline string to_string(int value)
{
	ostringstream temp;
	
	temp << value;
	return temp.str();
}

/*
 * Algorithm from http://oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
 */
inline void tokenize(const string& str, vector<int>& tokens, const string& delimiters = " ")
{
    // Skip delimiters at beginning.
    string::size_type lastPos=str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos=str.find_first_of(delimiters, lastPos);

    while(string::npos!=pos || string::npos!=lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(atoi(str.substr(lastPos, pos-lastPos).c_str()));
        // Skip delimiters.  Note the "not_of"
        lastPos=str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos=str.find_first_of(delimiters, lastPos);
    }
}

inline vector<int> get_elements_as_vector(string input)
{
	vector<int> result;
	
	tokenize(input, result, "-");
	
	return result;
}

inline string get_elements_as_string(vector<int> input)
{
	ostringstream result;
	int i;
	
	for(i=0; i<input.size(); ++i)
	{
		if(i>0)
		{
			result << "-";
		}
		result << input[i];
	}
	
	return result.str();
}

inline int get_parameter_type(vector<st_parameter> parameters, string name)
{
	int i;
	
	for(i=0; i<parameters.size(); ++i)
	{
		if(parameters[i].name.compare(name)==0)
		{
			return parameters[i].type;
		}
	}
}

#endif /*LIBM3_XML_DRIVER_H_*/
