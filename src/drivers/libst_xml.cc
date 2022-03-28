/* @STSHELL_LICENSE_START@
 *
 *      __  _______  ___________ 
 *     /  |/  / __ \/ ___/_  __/ 
 *    / /|_/ / / / /\__ \ / /    
 *   / /  / / /_/ /___/ // /    
 *  /_/  /_/\____//____//_/     
 * 
 * Multi-Objective System Tuner 
 * Copyright (c) 2007-2022 Politecnico di Milano
 * 
 * Development leader: Vittorio Zaccaria
 * Main developers: Vittorio Zaccaria, Gianluca Palermo, Fabrizio Castro
 *  
 * 
 * @STSHELL_LICENSE_END@ */
/* @M3EXPLORER_LICENSE_START@
 *
 * This file is part of the Multicube Explorer tool.
 *
 * Authors: Vittorio Zaccaria, Gianluca Palermo, Giovanni Mariani
 * Copyright (c) 2008, Politecnico di Milano, Universita' della Svizzera
 * italiana All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * Neither the name of Politecnico di Milano nor Universita' della Svizzera
 * Italiana nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @M3EXPLORER_LICENSE_END@ */

/*
 * Note: Significant and important parts of this source file have been
 * contributed by Alessandro Sivieri while at Politecnico di Milano (2008).
 */

/* @additional_authors @, Alessandro Sivieri (2008)@ */

#include <iostream>
#include <sstream>
#include <string>
#include <cerrno>
#include <cstdlib>
#include <map>
#include <cstring>
#include <st_conv.h>

/*
 * libxml is a C library, so we have to do this little hack
 * to make it work here, under C++
 */
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xmlschemas.h>
#include <libxml/tree.h>
#include <libxml/xmlstring.h>
#include <st_shell_variables.h>
#include <st_parser.h>
#include <st_sim_utils.h>
#include <st_driver.h>
#include <st_exception.h>
#include <st_design_space.h>
#include <st_common_utils.h>

using namespace std;

/*
 * libxml is a C library, so we have to do this little hack
 * to make it work here, under C++
 */
extern "C" {
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
class st_xml_exception : public st_exception {
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
class st_xml_rule {
public:
  st_xml_rule(xmlNodePtr);
  ~st_xml_rule();
  xmlNodePtr get_rule();
  void set_rule(xmlNodePtr);
  bool validate(st_point &) throw(st_xml_exception);

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
  st_object *parse(xmlNodePtr &node, st_point &point) throw(st_xml_exception);
};

/**
 * This abstract class represents a reader/writer for an XML
 * document; read/write methods have to be implemented by any
 * subclass. It also offers an helper method for XPath search
 * and the current version used in the documents.
 *
 * @author Sivieri Alessandro
 */
class st_xml_io {
public:
  virtual xmlDocPtr read(string) = 0;
  virtual void write(string) = 0;
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
class st_xml_design_space : public st_xml_io {
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
class st_xml_simulator : public st_xml_io {
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
  string relative_path;
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
class st_xml_validator {
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
class st_xml_driver : public st_driver {
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
  st_point *simulate(st_point &point, st_env *env);
  bool is_valid(st_point &, st_env *);
  string get_name();
  st_xml_driver() throw(st_xml_exception);
  ~st_xml_driver();
  st_design_space *get_design_space(st_env *) throw(st_xml_exception);
  string get_point_representation(st_env *, st_point *);
  bool is_thread_safe() { return false; }

private:
  st_xml_simulator *simulator;
  vector<st_xml_rule *> rules;
  xmlDocPtr launch(string, xmlDocPtr) throw(st_xml_exception);
};

/** I know this is ugly, but we need it to function properly for MPI */
static st_design_space *local_design_space;

inline bool is_exp2(int n) {
  if (n < 1)
    return false;
  float exp2 = log(n) / log(2);
  int res = (int)pow((float)2, exp2);
  return n == res;
}

inline int contains(st_vector *v, st_string str) {
  st_object *obj;

  for (int i = 0; i < v->size(); ++i) {
    *obj = v->get(i);
    if (is_a<st_string *>(obj)) {
      string str2 = to<st_string *>(obj)->get_string();
      if (str2.compare(str.get_string()) == 0) {
        return i;
      }
    }
  }
  return -1;
}

inline bool is_int(string constant) {
  ostringstream counterexample;
  int cvalue = atoi(constant.c_str());
  counterexample << cvalue;
  if (counterexample.str().compare(constant) ==
      0) // no errors in conversion, constant should be an integer
  {
    return true;
  } else // conversion error, constant should be a string
  {
    return false;
  }
}

inline string to_string(int value) {
  ostringstream temp;

  temp << value;
  return temp.str();
}

/*
 * Algorithm from
 * http://oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
 */
inline void tokenize(const string &str, vector<int> &tokens,
                     const string &delimiters = " ") {
  // Skip delimiters at beginning.
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos = str.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos) {
    // Found a token, add it to the vector.
    tokens.push_back(atoi(str.substr(lastPos, pos - lastPos).c_str()));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
}

inline vector<int> get_elements_as_vector(string input) {
  vector<int> result;

  tokenize(input, result, "-");

  return result;
}

inline string get_elements_as_string(vector<int> input) {
  ostringstream result;
  int i;

  for (i = 0; i < input.size(); ++i) {
    if (i > 0) {
      result << "-";
    }
    result << input[i];
  }

  return result.str();
}

inline int get_parameter_type(vector<st_parameter> parameters, string name) {
  int i;

  for (i = 0; i < parameters.size(); ++i) {
    if (parameters[i].name.compare(name) == 0) {
      return parameters[i].type;
    }
  }
  throw "XML/function did not terminate correctly";
}

/// class st_xml_io ///

xmlXPathObjectPtr st_xml_io::get_nodeset(xmlDocPtr doc, xmlChar *xpath) {
  xmlXPathContextPtr context;
  xmlXPathObjectPtr result;

  context = xmlXPathNewContext(doc);
  if (context == NULL) {
    return NULL;
  }
  result = xmlXPathEvalExpression(xpath, context);
  xmlXPathFreeContext(context);
  if (result == NULL) {
    return NULL;
  }
  if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
    xmlXPathFreeObject(result);
    return NULL;
  }
  return result;
}

int st_xml_get_time_out() {
  int timeout = 0;
  if (!current_environment.shell_variables.get_integer("simulation_timeout",
                                                       timeout))
    return 0;
  return timeout;
}

bool st_xml_enforce_version_1_3() {
  int is_1_3 = 0;
  if (!current_environment.shell_variables.get_integer(
          "backward_compatible_1_3", is_1_3))
    return false;
  if (is_1_3)
    return true;
  else
    return false;
}

string st_xml_get_apparent_version() {
  if (st_xml_enforce_version_1_3())
    return "1.3";
  else
    return "1.5";
}

bool st_xml_check_version(string doc_version) {
  bool doc_is_1_3 = (doc_version.compare("1.3") == 0);
  bool doc_is_1_4 = (doc_version.compare("1.4") == 0);
  bool doc_is_1_5 = (doc_version.compare("1.5") == 0);
  if (doc_is_1_3 || (doc_is_1_4 && !st_xml_enforce_version_1_3()) ||
      (doc_is_1_5 && !st_xml_enforce_version_1_3()))
    return true;
  else
    return false;
}

/// class st_xml_rule ///

st_xml_rule::st_xml_rule(xmlNodePtr rule) : rule(rule) {}

st_xml_rule::~st_xml_rule() { xmlFreeNodeList(this->rule); }

xmlNodePtr st_xml_rule::get_rule() { return this->rule; }

void st_xml_rule::set_rule(xmlNodePtr rule) {
  xmlFreeNodeList(this->rule);
  this->rule = rule;
}

bool st_xml_rule::validate(st_point &point) throw(st_xml_exception) {
  xmlNode *cur_node;
  st_object *result;

  // this little routine jumps all text nodes (that we have to ignore)
  // and finds out the only element node contained in a <rule/> tag
  for (cur_node = (this->rule)->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      result = this->parse(cur_node, point);
      break;
    }
  }
  if (result == NULL)
    return false; // never happens, exception thrown
  if (!is_a<st_integer *>(result) || !to<st_integer *>(result)->get_integer()) {
    delete result;
    return false;
  }

  delete result;
  return true;
}

/*
 * Please notice:
 * a) xmlChar is a simple char, so casts between the two are correct.
 * b) we DO NOT check every combination of objects, for example for 'equal'
 * tag we see only int-int and string-string, for 'expr' tag we check only
 * int-int, and that is because of a pre-checking of correctness of tags into
 * the XSD validation, and because in the last lines if result==NULL we throw
 * an exception; this produces two consequences: it works as a big ELSE
 * condition for everything not checked before, and we do not need to look for
 * thing!=NULL, because this function does not return NULL, it throws the
 * exception. c) libxml, like most XML libraries, considers even the \n, \r, \t
 * as valid nodes (of type text), but our driver completely ignores this kind of
 * tag because our specification does not contain any of them: please check this
 * as a possible source of bugs (even if now it should be correctly handled,
 * when we look for XML_ELEMENT_NODE), and pay attention if some valid text
 * nodes will be added to the specification for our input/output files.
 */
st_object *st_xml_rule::parse(xmlNodePtr &node,
                              st_point &point) throw(st_xml_exception) {
  string name = string((char *)node->name);
  xmlNodePtr *children;
  xmlNode *current = NULL, *if_current = NULL;
  xmlChar *attr;
  st_object *obj1 = NULL, *obj2 = NULL, *result = NULL;
  ostringstream temp;
  string *pname;
  int i, j, zero;

  // getting children count & elements
  for (i = 0, current = node->children; current; current = current->next) {
    if (current->type == XML_ELEMENT_NODE) {
      ++i;
    }
  }
  children = new xmlNodePtr[i];
  for (i = 0, current = node->children; current; current = current->next) {
    if (current->type == XML_ELEMENT_NODE) {
      children[i] = current;
      ++i;
    }
  }
  // switch
  if (name.compare("and") == 0) {
    zero = 1; // true: neutral element for boolean 'and'
    for (j = 0; j < i; ++j) {
      obj1 = parse(children[j], point);
      if (is_a<st_integer *>(obj1)) {
        zero = zero && to<st_integer *>(obj1)->get_integer();
      } else {
        /* pay attention: normally we do not do this 'else' part,
         * because if the is_a operator fails, result remains NULL,
         * thus the exception is thrown.
         * Here things are different: if is_a fails, this invocation
         * returns correctly because result is set anyway; so we
         * are forcing things to return false.
         */
        prs_display_error(
            "You have written a parameter or a constant with a string value in "
            "a boolean tag condition. Rule returns false.");
        zero = 0;
        break;
      }
      delete obj1;
      obj1 = NULL;
      // if we have a logic 'and' with one of the terms false,
      // we can safely return it without further processing
      if (!zero) {
        break;
      }
    }
    result = new st_integer(zero);
  } else if (name.compare("or") == 0) {
    zero = 0; // false: neutral element for boolean 'or'
    for (j = 0; j < i; ++j) {
      obj1 = parse(children[j], point);
      if (is_a<st_integer *>(obj1)) {
        zero = zero || to<st_integer *>(obj1)->get_integer();
      } else {
        /* pay attention: normally we do not do this 'else' part,
         * because if the is_a operator fails, result remains NULL,
         * thus the exception is thrown.
         * Here things are different: if is_a fails, this invocation
         * returns correctly because result is set anyway; so we
         * are forcing things to return false.
         */
        prs_display_error(
            "You have written a parameter or a constant with a string value in "
            "a boolean tag condition. Rule returns false.");
        zero = 0;
        break;
      }
      delete obj1;
      obj1 = NULL;
      // if we have a logic 'or' with one of the terms true,
      // we can safely return it without further processing
      if (zero) {
        break;
      }
    }
    result = new st_integer(zero);
  } else if (name.compare("not") == 0) {
    obj1 = parse(children[0], point); // only one child
    if (is_a<st_integer *>(obj1)) {
      result = new st_integer(!to<st_integer *>(obj1)->get_integer());
    }
  } else if (name.compare("equal") == 0) {
    obj1 = parse(children[0], point);
    obj2 = parse(children[1], point);
    if (is_a<st_integer *>(obj1) && is_a<st_integer *>(obj2)) {
      result = new st_integer(to<st_integer *>(obj1)->get_integer() ==
                              to<st_integer *>(obj2)->get_integer());
    } else if (is_a<st_string *>(obj1) && is_a<st_string *>(obj2)) {
      result = new st_integer(to<st_string *>(obj1)->get_string().compare(
                                  to<st_string *>(obj2)->get_string()) == 0);
    } // else ditto
  } else if (name.compare("not-equal") == 0) {
    obj1 = parse(children[0], point);
    obj2 = parse(children[1], point);
    if (is_a<st_integer *>(obj1) && is_a<st_integer *>(obj2)) {
      result = new st_integer(to<st_integer *>(obj1)->get_integer() !=
                              to<st_integer *>(obj2)->get_integer());
    } else if (is_a<st_string *>(obj1) && is_a<st_string *>(obj2)) {
      result = new st_integer(to<st_string *>(obj1)->get_string().compare(
                                  to<st_string *>(obj2)->get_string()) != 0);
    }
  } else if (name.compare("less") == 0) {
    obj1 = parse(children[0], point);
    obj2 = parse(children[1], point);
    if (is_a<st_integer *>(obj1) && is_a<st_integer *>(obj2)) {
      result = new st_integer(to<st_integer *>(obj1)->get_integer() <
                              to<st_integer *>(obj2)->get_integer());
    }
  } else if (name.compare("less-equal") == 0) {
    obj1 = parse(children[0], point);
    obj2 = parse(children[1], point);
    if (is_a<st_integer *>(obj1) && is_a<st_integer *>(obj2)) {
      result = new st_integer(to<st_integer *>(obj1)->get_integer() <=
                              to<st_integer *>(obj2)->get_integer());
    }
  } else if (name.compare("greater") == 0) {
    obj1 = parse(children[0], point);
    obj2 = parse(children[1], point);
    if (is_a<st_integer *>(obj1) && is_a<st_integer *>(obj2)) {
      result = new st_integer(to<st_integer *>(obj1)->get_integer() >
                              to<st_integer *>(obj2)->get_integer());
    }
  } else if (name.compare("greater-equal") == 0) {
    obj1 = parse(children[0], point);
    obj2 = parse(children[1], point);
    if (is_a<st_integer *>(obj1) && is_a<st_integer *>(obj2)) {
      result = new st_integer(to<st_integer *>(obj1)->get_integer() >=
                              to<st_integer *>(obj2)->get_integer());
    }
  } else if (name.compare("expr") == 0) {
    obj1 = parse(children[0], point);
    obj2 = parse(children[1], point);
    // getting operator and executing it
    xmlChar *tmp = xmlGetProp(node, (xmlChar *)"operator");
    string op = string((char *)tmp);
    xmlFree(tmp);
    if (is_a<st_integer *>(obj1) && is_a<st_integer *>(obj2)) {
      if (op.compare("+") == 0) {
        result = new st_integer(to<st_integer *>(obj1)->get_integer() +
                                to<st_integer *>(obj2)->get_integer());
      } else if (op.compare("-") == 0) {
        result = new st_integer(to<st_integer *>(obj1)->get_integer() -
                                to<st_integer *>(obj2)->get_integer());
      } else if (op.compare("*") == 0) {
        result = new st_integer(to<st_integer *>(obj1)->get_integer() *
                                to<st_integer *>(obj2)->get_integer());
      } else if (op.compare("/") == 0) {
        result = new st_integer(to<st_integer *>(obj1)->get_integer() /
                                to<st_integer *>(obj2)->get_integer());
      }
    }
  } else if (name.compare("parameter") == 0) {
    attr = xmlGetProp(node, (xmlChar *)"name");
    pname = new string((char *)attr);
    if (!local_design_space->ds_parameters_index.count(*pname)) {
      temp << "Parameter " << pname << " in rule has not been declared.";
      throw st_xml_exception(temp.str());
    }
    string str_value;
    str_value = local_design_space->get_parameter_representation(
        &current_environment, (point), *pname);
    if (is_int(str_value)) {
      result = new st_integer(atoi(str_value.c_str()));
    } else {
      result = new st_string(str_value);
    }
    delete pname;
    xmlFree(attr);
  } else if (name.compare("constant") == 0) {
    attr = xmlGetProp(node, (xmlChar *)"value");
    pname = new string((char *)attr);
    if (is_int(*pname)) {
      result = new st_integer(atoi(pname->c_str()));
    } else // conversion error, constant should be a string
    {
      result = new st_string(*pname);
    }
    delete pname;
    xmlFree(attr);
  } else if (name.compare("if") == 0) {
    obj1 = parse(children[0], point);
    if (is_a<st_integer *>(obj1)) {
      if (to<st_integer *>(obj1)->get_integer()) // 'then' part
      {
        for (if_current = children[1]->children; if_current;
             if_current = if_current->next) {
          if (if_current->type == XML_ELEMENT_NODE) {
            // always executed: 'then' has just one tag by spec
            obj2 = parse(if_current, point);
            break;
          }
        }
      } else if (i > 2) // if there is a third child, it must be 'else' (XSD)
      {
        for (if_current = children[2]->children; if_current;
             if_current = if_current->next) {
          if (if_current->type == XML_ELEMENT_NODE) {
            // always executed: 'else' has just one tag by spec
            obj2 = parse(if_current, point);
            break;
          }
        }
      } else // there is not a third child, we are lacking 'else'...
      {
        result = new st_integer(1);
      }
      if (is_a<st_integer *>(obj2)) {
        result = new st_integer(to<st_integer *>(obj2)->get_integer());
      }
    }
  }

  // destroy any temporary parameter and return
  if (obj1 != NULL)
    delete obj1;
  if (obj2 != NULL)
    delete obj2;
  delete[] children;
  // this should not be NULL...
  if (result == NULL) {
    temp << "For tag " << name << " the result value is NULL!";
    throw st_xml_exception(temp.str());
  }
  return result;
}

/// class st_xml_validator ///

const string st_xml_validator::design_space_schema =
    "/schemas/multicube_design_space_def.xsd";
const string st_xml_validator::simulator_schema =
    "/schemas/multicube_simulator_interface.xsd";
st_xml_validator *st_xml_validator::design_space_validator = NULL;
st_xml_validator *st_xml_validator::simulator_validator = NULL;

bool st_xml_validator::validate(xmlDocPtr doc) {
  if (xmlSchemaValidateDoc(schema_valid_run, doc) == 0) {
    return true;
  } else {
    return false;
  }
}

st_xml_validator *
st_xml_validator::get_design_space_validator() throw(st_xml_exception) {
  if (design_space_validator == NULL) {
    design_space_validator =
        new st_xml_validator(st_xml_validator::design_space_schema);
  }

  return design_space_validator;
}

st_xml_validator *
st_xml_validator::get_simulator_validator() throw(st_xml_exception) {
  if (simulator_validator == NULL) {
    simulator_validator =
        new st_xml_validator(st_xml_validator::simulator_schema);
  }

  return simulator_validator;
}

void st_xml_validator::shutdown() {
  if (simulator_validator != NULL) {
    delete simulator_validator;
    simulator_validator = NULL;
  }
  if (design_space_validator != NULL) {
    delete design_space_validator;
    design_space_validator = NULL;
  }
}

st_xml_validator::~st_xml_validator() {
  xmlSchemaFreeValidCtxt(schema_valid_run);
  xmlSchemaFree(schema_run);
  xmlSchemaFreeParserCtxt(schema_context_run);
}

st_xml_validator::st_xml_validator(string filename) throw(st_xml_exception)
    : current_schema(filename) {
  ostringstream temp;
  st_object *obj;

  // here we create everything, only if this is our first invocation
  string path = st_get_current_build_path(&current_environment);
  temp << path << current_schema;
  // cout << "Using the following build path " << to<st_string
  // *>(obj)->get_string() << endl;
  schema_context_run = xmlSchemaNewParserCtxt(temp.str().c_str());
  if (schema_context_run == NULL) {
    throw st_xml_exception(
        "Unable to load schema parser context for validator");
  }
  xmlSchemaSetParserErrors(schema_context_run,
                           (xmlSchemaValidityErrorFunc)fprintf,
                           (xmlSchemaValidityWarningFunc)fprintf, stderr);
  schema_run = xmlSchemaParse(schema_context_run);
  if (schema_run == NULL) {
    xmlSchemaFreeParserCtxt(schema_context_run);
    throw st_xml_exception("Unable to load schema for validator");
  }
  schema_valid_run = xmlSchemaNewValidCtxt(schema_run);
  if (schema_valid_run == NULL) {
    xmlSchemaFree(schema_run);
    xmlSchemaFreeParserCtxt(schema_context_run);
    throw st_xml_exception("Unable to load schema valid context for validator");
  }
  xmlSchemaSetValidErrors(schema_valid_run, (xmlSchemaValidityErrorFunc)fprintf,
                          (xmlSchemaValidityWarningFunc)fprintf, stderr);
}

/// class st_xml_design_space ///

st_xml_design_space::st_xml_design_space() {}

st_xml_design_space::~st_xml_design_space() {}

void st_xml_design_space::write(string filename) throw(st_xml_exception) {
  throw st_xml_exception("Unsupported operation.");
}

xmlDocPtr st_xml_design_space::read(string filename) throw(st_xml_exception) {
  int i;
  xmlDocPtr doc;
  xmlChar *xpath;
  xmlXPathObjectPtr xpath_set;
  xmlNodeSetPtr nodeset;
  string input, schema, bpath;
  ostringstream temp;
  st_object *obj;

  // loading XML input and validating against the specified schema
  doc = xmlParseFile(filename.c_str());
  if (doc == NULL) {
    throw st_xml_exception("Unable to open the given input file.");
  }
  if (!st_xml_validator::get_design_space_validator()->validate(doc)) {
    throw st_xml_exception("Error during validation.");
  }

  // version check

  xmlChar *value = xmlGetProp(doc->children, (xmlChar *)"version");
  string docversion = string((char *)value);
  xmlFree(value);

  if (!st_xml_check_version(docversion)) {
    temp << "Document version " << docversion
         << "does not match the actual one: " << st_xml_get_apparent_version();
    throw st_xml_exception(temp.str());
  }

  return doc;
}

int st_xml_times_to_repeat_metrics(xmlDocPtr doc, st_xml_design_space *ds) {
  int repeat = 0;
  xmlChar *xpath;
  xmlXPathObjectPtr xpath_set;
  xpath =
      (xmlChar
           *)"/*[local-name()='design_space']/*[local-name()='system_metrics']";
  xpath_set = ds->get_nodeset(doc, xpath);

  if (xpath_set == NULL) {
    cout << "No path found" << endl;
    return 0;
  }

  xmlNodeSetPtr nodeset;
  nodeset = xpath_set->nodesetval;

  for (int i = 0; i < nodeset->nodeNr; ++i) {
    xmlChar *tmp = xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"repeat");
    if (tmp != NULL) {
      repeat = atoi((char *)tmp);
      xmlFree(tmp);
    }
  }
  xmlXPathFreeObject(xpath_set);
  return repeat;
}

void st_xml_design_space::fill(st_design_space *design_space) throw(
    st_xml_exception) {
  int i;
  xmlChar *xpath;
  xmlXPathObjectPtr xpath_set;
  xmlNodeSetPtr nodeset;
  string input, schema, bpath;
  ostringstream temp;
  st_object *obj;

  xpath = (xmlChar *)"/*[local-name()='design_space']/"
                     "*[local-name()='parameters']/*[local-name()='parameter']";
  xpath_set = this->get_nodeset(doc, xpath);
  if (xpath_set == NULL) {
    throw st_xml_exception("Cannot load parameters.");
  }
  nodeset = xpath_set->nodesetval;
  for (i = 0; i < nodeset->nodeNr; ++i) {
    this->add_parameter(design_space, &current_environment, doc,
                        nodeset->nodeTab[i]);
  }
  xmlXPathFreeObject(xpath_set);

  // loading metrics
  xpath =
      (xmlChar
           *)"/*[local-name()='design_space']/*[local-name()='system_metrics']/"
             "*[local-name()='system_metric']";
  xpath_set = this->get_nodeset(doc, xpath);
  if (xpath_set == NULL) {
    throw st_xml_exception("Cannot load metrics.");
  }
  nodeset = xpath_set->nodesetval;

  int times = st_xml_times_to_repeat_metrics(doc, this);

  if (times == 0) {
    for (i = 0; i < nodeset->nodeNr; ++i) {
      xmlChar *tmp = xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"name");
      string tmp_str1 = string((char *)tmp);
      xmlFree(tmp);
      tmp = xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"unit");
      string tmp_str2 = string((char *)tmp);
      xmlFree(tmp);

      design_space->insert_metric(&current_environment, tmp_str1, tmp_str2);
    }
  } else {
    for (int j = 0; j < times; j++) {
      for (i = 0; i < nodeset->nodeNr; ++i) {
        xmlChar *tmp = xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"name");
        string tmp_str1 = string((char *)tmp);
        xmlFree(tmp);
        tmp = xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"unit");
        string tmp_str2 = string((char *)tmp);
        xmlFree(tmp);

        tmp_str1 = tmp_str1 + "_" + st_itoa(j);
        design_space->insert_metric(&current_environment, tmp_str1, tmp_str2);
      }
    }
  }
  xmlXPathFreeObject(xpath_set);

  // loading future simulator launch configurations
  xpath =
      (xmlChar *)"/*[local-name()='design_space']/*[local-name()='simulator']/"
                 "*[local-name()='simulator_executable']";
  xpath_set = this->get_nodeset(doc, xpath);
  if (xpath_set == NULL) {
    throw st_xml_exception("Cannot load simulation configurations.");
  }
  nodeset = xpath_set->nodesetval;
  for (i = 0; i < nodeset->nodeNr; ++i) {
    if (i == 0) {
      xmlChar *tmp = xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"path");
      this->simulator = string((char *)tmp);
      xmlFree(tmp);
    } else {
      prs_display_message("WARNING: Design space description contains more "
                          "than one simulator; the first one will be used.");
    }
  }
  xmlXPathFreeObject(xpath_set);

  // loading rules
  xpath = (xmlChar *)"/*[local-name()='design_space']/*[local-name()='rules']/"
                     "*[local-name()='rule']";
  xpath_set = this->get_nodeset(doc, xpath);
  if (xpath_set != NULL) {
    // rules may be absent (at least for debug purposes)
    nodeset = xpath_set->nodesetval;
    for (i = 0; i < nodeset->nodeNr; ++i) {
      // we save a copy of each <rule /> subtree
      this->rules.push_back(
          new st_xml_rule(xmlCopyNodeList(nodeset->nodeTab[i])));
    }
    xmlXPathFreeObject(xpath_set);
  }

  // closing XML input and leave
  xmlFreeDoc(doc);
  xmlCleanupParser();
}

vector<st_xml_rule *> st_xml_design_space::get_rules() { return this->rules; }

string st_xml_design_space::get_simulator() { return this->simulator; }

void st_xml_design_space::add_parameter(st_design_space *design_space,
                                        st_env *env, xmlDocPtr doc,
                                        xmlNode *node) {
  xmlChar *value;
  ostringstream temp;
  vector<string> el_list;
  int max, min, step = 1, i, j;
  xmlXPathObjectPtr xpath_set;
  xmlNodeSetPtr nodeset;

  value = xmlGetProp(node, (xmlChar *)"type");
  if (xmlStrEqual(value, (xmlChar *)"integer")) {
    bool has_multiple_instances = false;
    int instances = 1;
    // optional 'instances' parameter
    xmlFree(value);
    value = xmlGetProp(node, (xmlChar *)"instances");
    if (value != NULL) {
      if (st_xml_enforce_version_1_3()) {
        prs_display_error("Cannot use multiple instances because backward "
                          "compatibility is set");
      } else {
        has_multiple_instances = true;
        instances = atoi((char *)value);
        if (instances <= 1) {
          temp << "Parameter " << node->name
               << " instances invalid. Assuming 1 instance.";
          prs_display_error(temp.str());
          has_multiple_instances = false;
        }
      }
    }
    xmlFree(value);

    // parameter 'min' (mandatory)
    value = xmlGetProp(node, (xmlChar *)"min");
    if (value == NULL) {
      temp << "Parameter " << node->name << " has no attribute 'min'.";
      prs_display_error(temp.str());
      return;
    }
    min = atoi((char *)value);
    xmlFree(value);

    // parameter 'max' (mandatory)
    value = xmlGetProp(node, (xmlChar *)"max");
    if (value == NULL) {
      temp << "Parameter " << node->name << " has no attribute 'max'.";
      prs_display_error(temp.str());

      return;
    }
    max = atoi((char *)value);
    xmlFree(value);

    // parameter 'step' (if present)
    value = xmlGetProp(node, (xmlChar *)"step");
    if (value != NULL) {
      step = atoi((char *)value);
    }
    xmlFree(value);

    xmlChar *tmp = xmlGetProp(node, (xmlChar *)"name");
    string par_name = string((char *)tmp);
    xmlFree(tmp);

    if (step != 1) {
      for (i = min; i <= max; i = i + step) {
        ostringstream int2string;
        int2string << i;
        el_list.push_back(int2string.str());
      }
      if (has_multiple_instances) {
        for (int mi = 1; mi <= instances; mi++) {
          ostringstream int2string;
          int2string << mi;
          string suffix = int2string.str();
          design_space->insert_scalar(env, par_name + "_" + suffix,
                                      ST_SCALAR_TYPE_LIST, min, max, el_list);
        }
      } else
        design_space->insert_scalar(env, par_name, ST_SCALAR_TYPE_LIST, min,
                                    max, el_list);
    } else {
      if (has_multiple_instances) {
        for (int mi = 1; mi <= instances; mi++) {
          ostringstream int2string;
          int2string << mi;
          string suffix = int2string.str();
          design_space->insert_scalar(env, par_name + "_" + suffix,
                                      ST_SCALAR_TYPE_INTEGER, min, max,
                                      vector<string>());
        }
      } else
        design_space->insert_scalar(env, par_name, ST_SCALAR_TYPE_INTEGER, min,
                                    max, vector<string>());
    }
  } else if (xmlStrEqual(value, (xmlChar *)"string")) {
    xmlFree(value);
    ostringstream query;
    xmlChar *tmp = xmlGetProp(node, (xmlChar *)"name");
    string tmp_str = string((char *)tmp);
    xmlFree(tmp);
    query << "/*[local-name()='design_space']/*[local-name()='parameters']/"
             "*[local-name()='parameter'][@name='"
          << tmp_str << "']/*[local-name()='item']";
    xpath_set = this->get_nodeset(doc, (xmlChar *)query.str().c_str());
    if (xpath_set == NULL) {
      temp << "Parameter " << node->name
           << " has no list of possibile string values.";
      prs_display_error(temp.str());
      return;
    }
    nodeset = xpath_set->nodesetval;
    for (i = 0; i < nodeset->nodeNr; ++i) {
      xmlChar *tmp = xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"value");
      string par_val = string((char *)tmp);
      xmlFree(tmp);
      el_list.push_back(par_val);
    }
    tmp = xmlGetProp(node, (xmlChar *)"name");
    tmp_str = string((char *)tmp);
    xmlFree(tmp);
    design_space->insert_scalar(env, tmp_str, ST_SCALAR_TYPE_LIST, 0, 0,
                                el_list);

  } else if (xmlStrEqual(value, (xmlChar *)"boolean")) {
    xmlFree(value);
    el_list.push_back("0");
    el_list.push_back("1");
    xmlChar *tmp = xmlGetProp(node, (xmlChar *)"name");
    string tmp_str = string((char *)tmp);
    xmlFree(tmp);
    design_space->insert_scalar(env, tmp_str, ST_SCALAR_TYPE_LIST, 0, 0,
                                el_list);
  } else if (xmlStrEqual(value, (xmlChar *)"exp2")) {
    xmlFree(value);
    // parameter 'min' (mandatory)
    value = xmlGetProp(node, (xmlChar *)"min");
    if (value == NULL) {
      temp << "Parameter " << node->name << " has no attribute 'min'.";
      prs_display_error(temp.str());
      return;
    }
    min = atoi((char *)value);
    xmlFree(value);
    // parameter 'max' (mandatory)
    value = xmlGetProp(node, (xmlChar *)"max");
    if (value == NULL) {
      temp << "Parameter " << node->name << " has no attribute 'max'.";
      prs_display_error(temp.str());
      return;
    }
    max = atoi((char *)value);
    xmlFree(value);
    if (!is_exp2(min) || !is_exp2(max)) {
      temp << "Parameter " << node->name << " has min or max not a power of 2.";
      prs_display_error(temp.str());
      return;
    }
    // calculating and appending everything
    for (i = min; i <= max; i = i * 2) {
      ostringstream int2string;
      int2string << i;
      el_list.push_back(int2string.str());
    }
    xmlChar *tmp = xmlGetProp(node, (xmlChar *)"name");
    string par_name = string((char *)tmp);
    xmlFree(tmp);
    design_space->insert_scalar(env, par_name, ST_SCALAR_TYPE_LIST, 0, 0,
                                el_list);
  } else if (xmlStrEqual(value, (xmlChar *)"permutation")) {
    xmlFree(value);
    value = xmlGetProp(node, (xmlChar *)"dimension");
    if (value == NULL) {
      temp << "Parameter " << node->name
           << " does not have the property 'dimension'.";
      prs_display_error(temp.str());
      return;
    }
    int int_dimension;
    string str_dimension;
    if (is_int(string((char *)value))) {
      str_dimension = "";
      int_dimension = atoi((char *)value);
    } else {
      str_dimension = string((char *)xmlStrsub(value, 1, xmlStrlen(value) - 1));
      int_dimension = 0;
    }
    xmlFree(value);
    xmlChar *tmp = xmlGetProp(node, (xmlChar *)"name");
    string tmp_str = string((char *)tmp);
    xmlFree(tmp);
    design_space->insert_permutation(env, tmp_str, int_dimension == 0,
                                     str_dimension, int_dimension);
  } else // on_off_mask
  {
    xmlFree(value);
    value = xmlGetProp(node, (xmlChar *)"dimension");
    if (value == NULL) {
      temp << "Parameter " << node->name
           << " does not have the property 'dimension'.";
      prs_display_error(temp.str());
      return;
    }
    int int_dimension;
    string str_dimension;
    if (is_int(string((char *)value))) {
      str_dimension = "";
      int_dimension = atoi((char *)value);
    } else {
      str_dimension = string((char *)xmlStrsub(value, 1, xmlStrlen(value) - 1));
      int_dimension = 0;
    }
    xmlFree(value);
    xmlChar *value2 = xmlGetProp(node, (xmlChar *)"on_set_size");
    int fixed_on_set_size;
    string on_set_dependent_parameter;
    if (value2 == NULL) {
      fixed_on_set_size = 0;
      on_set_dependent_parameter = "";
    } else {
      if (is_int(string((char *)value2))) {
        on_set_dependent_parameter = "";
        fixed_on_set_size = atoi((char *)value2);
      } else {
        on_set_dependent_parameter =
            string((char *)xmlStrsub(value2, 1, xmlStrlen(value2) - 1));
        fixed_on_set_size = 0;
      }
    }
    xmlChar *tmp = xmlGetProp(node, (xmlChar *)"name");
    string tmp_str = string((char *)tmp);
    xmlFree(tmp);
    design_space->insert_on_off_mask(
        env, tmp_str, int_dimension == 0, str_dimension, int_dimension,
        value2 == NULL, fixed_on_set_size == 0, on_set_dependent_parameter,
        fixed_on_set_size);
    if (value2 != NULL)
      xmlFree(value2);
  }
}

xmlDocPtr st_xml_design_space::get_doc() { return this->doc; }

void st_xml_design_space::set_doc(xmlDocPtr doc) { this->doc = doc; }

/// class st_xml_simulator ///

string st_xml_simulator::directory = "";
string st_xml_simulator::relative_directory = "";

unsigned int st_xml_simulator::counter = 1;

st_xml_simulator::st_xml_simulator(string simulator) : simulator(simulator) {
  if (st_xml_simulator::directory.compare("") == 0) {
    st_xml_simulator::init();
  }
}

st_xml_simulator::~st_xml_simulator() {}

xmlDocPtr st_xml_simulator::read(string filename) throw(st_xml_exception) {
  xmlDocPtr output;
  ostringstream temp;

  output = xmlParseFile(filename.c_str());
  if (output == NULL) {
    temp << "Unable to open the given input file: " << filename;
    throw st_xml_exception(temp.str());
  }
  if (!st_xml_validator::get_simulator_validator()->validate(output)) {
    throw st_xml_exception("Error during validation.");
  }
  // version check
  xmlChar *tmp = xmlGetProp(output->children, (xmlChar *)"version");
  string docversion = string((char *)tmp);
  xmlFree(tmp);
  if (!st_xml_check_version(docversion)) {
    temp << "Document version " << docversion
         << "does not match the actual one: " << st_xml_get_apparent_version();
    throw st_xml_exception(temp.str());
  }

  return output;
}

string get_current_dir();

void st_xml_simulator::write(string filename) throw(st_xml_exception) {
  xmlDocPtr input;
  xmlNodePtr root, node, item;
  map<string, string>::iterator it;
  int i = 0, j;
  st_design_space *design_space = local_design_space;

  // input preparation
  input = xmlNewDoc((xmlChar *)"1.0");
  root = xmlNewNode(NULL, (xmlChar *)"simulator_input_interface");
  xmlNewProp(root, (xmlChar *)"xmlns", (xmlChar *)"http://www.multicube.eu/");
  xmlNewProp(root, (xmlChar *)"version",
             (xmlChar *)st_xml_get_apparent_version().c_str());
  xmlDocSetRootElement(input, root);
  for (it = parameters.begin(); it != parameters.end(); it++) {
    node = xmlNewChild(root, NULL, (xmlChar *)"parameter", NULL);
    xmlNewProp(node, (xmlChar *)"name", (xmlChar *)it->first.c_str());
    int type = get_parameter_type(design_space->ds_parameters, it->first);
    if (type == ST_DS_PERMUTATION) {
      vector<int> permutation = get_elements_as_vector(it->second);
      for (j = 0; j < permutation.size(); ++j) {
        item = xmlNewChild(node, NULL, (xmlChar *)"item", NULL);
        string jj = std::to_string(j + 1);
        string vj = std::to_string(permutation[j]);
        xmlNewProp(item, (xmlChar *)"position", (xmlChar *)jj.c_str());
        xmlNewProp(item, (xmlChar *)"value", (xmlChar *)vj.c_str());
      }
    } else if (type == ST_DS_ON_OFF_MASK) {
      vector<int> mask = get_elements_as_vector(it->second);
      for (j = 0; j < mask.size(); ++j) {
        item = xmlNewChild(node, NULL, (xmlChar *)"item", NULL);
        string jj = std::to_string(j + 1);
        string vj = std::to_string(mask[j]);
        xmlNewProp(item, (xmlChar *)"index", (xmlChar *)jj.c_str());
        xmlNewProp(item, (xmlChar *)"value", (xmlChar *)vj.c_str());
      }
    } else // scalar value, we can insert it as is
    {
      xmlNewProp(node, (xmlChar *)"value", (xmlChar *)it->second.c_str());
    }
    ++i;
  }
  xmlSaveFormatFileEnc(filename.c_str(), input, "UTF-8", 1);
}

string get_current_dir() {
  char sz[400];
  string s = getcwd(sz, 400);
  return s;
}

static string invocation_dir;
extern string local_build_path;

/*
 * We are using a system() call because I had some problems in using
 * the standard fork()/exec() paradigm, obtaining obscure errors.
 * We do a fork, even if not strictly required by that call, because
 * we need a unique pid for each call to create unique input/output XML
 * pairs. So for each simulate() call we create two children.
 */
void st_xml_simulator::run(map<string, string> parameters) throw() {
  int status = 0;
  ostringstream directory, filename, filename_out, temp, temp2, rdir;
  xmlDocPtr output = NULL;
  xmlChar *xpath, *attr, *attr2;
  time_t t;
  struct tm *tmp;
  char tms[200];
  bool error = false;
  string relative_path_p = "NA";
  xmlXPathObjectPtr xpath_set = NULL;
  xmlNodeSetPtr nodeset;
  int i;
  string *pname, *pname2;
  st_object *obj;

  try {
    this->parameters = parameters;
    this->metrics.clear();
    t = time(NULL);
    tmp = localtime(&t);
    strftime(tms, sizeof(tms), ".%d-%m-%Y.%H-%M-%S.xml", tmp);

    // writing input file
    directory << st_xml_simulator::directory << "/" << this->counter << "/";
    rdir << st_xml_simulator::relative_directory << "/" << this->counter;

    relative_path_p = rdir.str();
    filename << "input" << tms;
    filename_out << "output" << tms;

    // we need to ignore errno 17: it happens without an explicit cause
    // cout << "Creating " << directory.str() << endl;

    if (mkdir(directory.str().c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0 &&
        errno != 17) {
      temp << "Unable to create temporary directory: " << errno;
      throw st_xml_exception(temp.str());
    }

    // cout << "Moving from " << get_dir() << " to " << directory.str() << endl;

    if (chdir(directory.str().c_str()) != 0) {
      temp << "Unable to chdir to the temporary directory: " << errno;
      throw st_xml_exception(temp.str());
    }

    this->write(filename.str());

    string bpath = st_get_current_build_path(&current_environment);

    int timeout = st_xml_get_time_out();

    temp << simulator << " --xml_system_configuration=" << filename.str()
         << " --xml_system_metrics=" << filename_out.str()
         << " --reference_xsd=" << bpath << st_xml_validator::simulator_schema;
    if (timeout != 0) {
      temp << " --timeout=" << timeout;
    }

//#define DEBUG
#if defined(DEBUG)
    cout << "Simulator running into working directory: " << get_current_dir()
         << endl;
    cout << "Invoking with: " << temp.str() << endl;
#endif
    string cmd = temp2.str() + string("/bin/bash -c \"") + temp.str() + "\"";
//#define DEBUG
#if defined(DEBUG)
    cout << "Command line '" << cmd << "'" << endl;
#endif
    status = system(cmd.c_str());

    this->counter++;
    if (WEXITSTATUS(status) != 0) {
      temp << "The simulator returned an error code: " << WEXITSTATUS(status);
      throw st_xml_exception(temp.str());
    }
    // cout << "Trying to read " << get_dir() << endl;
    output = this->read(filename_out.str());
    if (chdir(invocation_dir.c_str()) != 0) {
      temp << "Unable to chdir back to the original directory: " << errno;
      throw st_xml_exception(temp.str());
    }
    // try normal output results
    xpath = (xmlChar *)"/*[local-name()='simulator_output_interface']/"
                       "*[local-name()='system_metric']";
    xpath_set = this->get_nodeset(output, xpath);
    if (xpath_set == NULL) {
      // ok, maybe we have an error...
      xpath = (xmlChar *)"/*[local-name()='simulator_output_interface']/"
                         "*[local-name()='error']";
      xpath_set = this->get_nodeset(output, xpath);
      if (xpath_set == NULL) {
        // no, something wrong happened somewhere... anyway, we
        // should not be here because of a correct validation
        throw st_xml_exception("Cannot load parameters.");
      }
      error = true;
    }
    nodeset = xpath_set->nodesetval;
    for (i = 0; i < nodeset->nodeNr; ++i) {
      if (error) {
        xmlChar *tmp = xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"reason");
        string tmp_str = string((char *)tmp);
        xmlFree(tmp);

        if (xmlStrcmp(xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"kind"),
                      (xmlChar *)"fatal") == 0) {
          this->error_code = ST_POINT_FATAL_ERROR;
          this->error_description = tmp_str;
          this->relative_path = relative_path_p;
        } else {
          this->error_code = ST_POINT_NON_FATAL_ERROR;
          this->error_description = tmp_str;
          this->relative_path = relative_path_p;
        }
      } else {
        attr = xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"name");
        pname = new string((char *)attr);
        attr2 = xmlGetProp(nodeset->nodeTab[i], (xmlChar *)"value");
        pname2 = new string((char *)attr2);
        double value = atof(pname2->c_str());
        metrics.insert(pair<string, double>(string(*pname), value));
        delete pname;
        delete pname2;
        xmlFree(attr);
        xmlFree(attr2);
        this->error_code = ST_POINT_NO_ERROR;
        this->relative_path = relative_path_p;
      }
    }
    xmlXPathFreeObject(xpath_set);
    xmlFreeDoc(output);
  } catch (st_xml_exception e) {
    if (xpath_set != NULL) {
      xmlXPathFreeObject(xpath_set);
    }
    if (output != NULL) {
      xmlFreeDoc(output);
    }
    /** We should make it non-fatal, in the case that the simulator is not
     * available */
    this->error_code = ST_POINT_NON_FATAL_ERROR;
    this->error_description = string(e.what());
    this->relative_path = relative_path_p;
  }
  chdir(invocation_dir.c_str());
}

map<string, double> st_xml_simulator::get_metrics() { return metrics; }

string st_xml_simulator::get_relative_path() { return relative_path; }

int st_xml_simulator::get_error_code() { return error_code; }

string st_xml_simulator::get_error_description() { return error_description; }

map<string, string> st_xml_simulator::get_parameters() { return parameters; }

void st_xml_simulator::init() throw(st_xml_exception) {
  ostringstream dir, temp, rdir;

  invocation_dir = get_current_dir();
  dir << invocation_dir << "/most.node." << st_get_unique_string_identifier();
  rdir << "/most.node." << st_get_unique_string_identifier();
  st_xml_simulator::directory = dir.str();
  st_xml_simulator::relative_directory = rdir.str();
  // we need to ignore errno 17: it happens without an explicit cause
  if (mkdir(st_xml_simulator::directory.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) !=
          0 &&
      errno != 17) {
    temp << "Unable to create temporary directory: " << errno;
    throw st_xml_exception(temp.str());
  }
}

bool st_xml_simulator::del_element(const char *name) {
  DIR *tdir;
  struct dirent *act_file;

  tdir = opendir(name);
  if (tdir != NULL) {
    act_file = readdir(tdir);
    while (act_file != NULL) {
      if (strcmp(act_file->d_name, ".") != 0 &&
          strcmp(act_file->d_name, "..") != 0) {
        // deleting each file
        if (act_file->d_type != DT_DIR) {
          ostringstream file;
          file << name << "/" << act_file->d_name;
          remove(file.str().c_str());
        } else // recursion
        {
          ostringstream temp;
          temp << name << "/" << act_file->d_name;
          if (!st_xml_simulator::del_element(temp.str().c_str())) {
            return false;
          }
        }
      }
      act_file = readdir(tdir);
    }
    closedir(tdir);
    // deleting empty directory
    remove(name);
    return true;
  } else {
    return false;
  }
}

void st_xml_simulator::shutdown() {
  int clean = 0;
  bool found = current_environment.shell_variables.get_integer(
      "clean_directories_on_exit", clean);
  st_object *obj;

  if (!found || clean) {
    if (!st_xml_simulator::del_element(st_xml_simulator::directory.c_str())) {
      cerr << "Unable to delete temporary directory" << endl;
    }
  }
}

/// class st_xml_driver ///

string st_xml_driver::get_information() {
  string info = "";
  info.append("XML driver written by A. Sivieri");
  return info;
}

st_design_space *
st_xml_driver::get_design_space(st_env *env) throw(st_xml_exception) {
  return local_design_space;
}

bool st_xml_driver::is_valid(st_point &point, st_env *env) {
  int i;

  if (this->rules.size() == 0) {
    return true;
  }
  for (i = 0; i < this->rules.size(); i++) {
    if (!this->rules[i]->validate(point)) {
      return false;
    }
  }

  return true;
}

st_point *st_xml_driver::simulate(st_point &point, st_env *env) {
  st_point *simulated_point = new st_point(point);
  if (!is_valid(point, env)) {
    simulated_point->set_error(ST_POINT_NON_FATAL_ERROR,
                               "Point violates design space rules");
    return simulated_point;
  }
  st_design_space *design_space = local_design_space;
  map<string, string> parameters;
  st_vector statistics, metrics;
  int i;
  map<string, double>::iterator it;

  // launch work
  for (i = 0; i < design_space->ds_parameters.size(); ++i) {
    st_parameter par = design_space->ds_parameters[i];
    if (par.type == ST_DS_PERMUTATION) {
      vector<int> permutation =
          design_space->get_permutation(env, &point, par.name);
      parameters.insert(
          pair<string, string>(par.name, get_elements_as_string(permutation)));
    } else if (par.type == ST_DS_ON_OFF_MASK) {
      vector<int> mask = design_space->get_mask(env, &point, par.name);
      parameters.insert(
          pair<string, string>(par.name, get_elements_as_string(mask)));
    } else // scalar value, we can insert it as is
    {
      // if the list of this scalar is empty, then we have an integer
      // with step 1, so the point value is the one requested
      if (design_space->scalar_parameters[par.name].list.size() == 0) {
        parameters.insert(
            pair<string, string>(par.name, std::to_string(point[i])));
      } else // otherwise, let's pick up the list element
      {
        parameters.insert(pair<string, string>(
            par.name,
            design_space->scalar_parameters[par.name].list[point[i]]));
      }
    }
  }
  simulator->run(parameters);
  // cout << " Current directory is " << get_current_dir() << endl;
  if (simulator->get_error_code() == ST_POINT_NO_ERROR) {
    for (i = 0; i < design_space->metric_names.size(); ++i) {
      double value = simulator->get_metrics()[design_space->metric_names[i]];
      st_double obj(value);
      metrics.insert(i, obj);
    }
    st_string rp(simulator->get_relative_path());
    simulated_point->set_properties("metrics", metrics);
    simulated_point->set_properties("statistics", statistics);
    simulated_point->set_properties("rpath", rp);
  } else {
    simulated_point->set_error(simulator->get_error_code(),
                               simulator->get_error_description());
    st_string rp(simulator->get_relative_path());
    simulated_point->set_properties("rpath", rp);
  }

  return simulated_point;
}

string st_xml_driver::get_name() {
  string name = "xml_driver";
  return name;
}

st_xml_driver::st_xml_driver() throw(st_xml_exception) {
  prs_display_message("Loading the xml_driver");
  st_xml_simulator::init();
  local_design_space = new st_design_space();
  st_xml_design_space reader;
  string design_space_file;
  bool found = current_environment.shell_variables.get_string(
      "xml_design_space_file", design_space_file);
  this->simulator = NULL;
  if (!found) {
    prs_display_error("Please define an xml_design_space_file before "
                      "instantiating the XML driver");
    return;
  }
  reader.set_doc(reader.read(design_space_file));
  reader.fill(local_design_space);
  this->simulator = new st_xml_simulator(reader.get_simulator());
  this->rules = reader.get_rules();
}

st_xml_driver::~st_xml_driver() {
  prs_display_message("Removing xml_driver");
  int i;
  for (i = 0; i < this->rules.size(); ++i) {
    delete this->rules[i];
  }
  st_xml_validator::shutdown();
  st_xml_simulator::shutdown();
  if (this->simulator)
    delete this->simulator;
  // this will be the last call, to be sure to clean up everything
  xmlCleanupParser();
}

/// class st_xml_exception ///

st_xml_exception::st_xml_exception(string msg)
    : st_exception("XML exception: " + msg) {}

st_xml_exception::~st_xml_exception() throw() {}

/// extra stuff ///

extern "C" {

st_xml_driver *drv_generate_driver() {
  prs_display_message("Creating the xml_driver");
  return new st_xml_driver();
}
}
