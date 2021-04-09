/* @STSHELL_LICENSE_START@
 *
 * Authors: Vittorio Zaccaria, Gianluca Palermo
 * Copyright (c) 2007 Politecnico di Milano
 *  
 * This file is confidential property of Politecnico di Milano. 
 * Distribution outside Politecnico di Milano is currently not allowed.
 *
 * @STSHELL_LICENSE_END@ */
/*
 */

#include <string>
#include <map>
#include <set>
#include <list>
#include <iostream>
#include <fstream>
#include "libxml_m3.h"
#include <string.h>
#include <cstdlib>


string arg_parser::look_for_option_name(string short_option)
{
  option_list::iterator i;
  for(i = options.begin();
      i != options.end();
      i++)
  {
    if(i->second.short_option_name == short_option)
      return i->first;
  }
  return "";
}

void arg_parser::insert(const char *option_name, const char short_option, const char *argument, const char *comment, arg_parser_item_type type)
{
  char opt[2];

  opt[1]='\0';
  opt[0]= short_option;

  arg_parser_item item;
  item.option_name = option_name;
  item.short_option_name = opt;
  item.argument_name = argument ? argument: "";
  item.comment = comment ? comment: "";
  item.type = type;

  options.insert(pair<string, arg_parser_item>(string(option_name), item));
}

bool arg_parser::option_is_set(string option_name)
{
  return (options[option_name].values.size() != 0);
}

string arg_parser::get_option_value(string option_name)
{
  return (*(options[option_name].values.begin()));
}

bool arg_parser::process_options(int argc, char **argv)
{
  for(int i=1; i<argc; i++)
  {
    bool complex_option=false;
    int  eq=0;

    char tmpstr1[200];
    char tmpstr2[200];

    string opt; 
    string val; 

    tmpstr1[0]='\0';
    tmpstr2[0]='\0';

    if(!strncmp(argv[i],"--",2))
    {
      int s=0;
      argv[i]=argv[i]+2;
      for(s=0; s<strlen(argv[i]); s++)
      {
        if(argv[i][s]=='=')
        {
          complex_option=true;
          eq=s;	
        }
      }	

      if(complex_option)
      {
        int sz = strlen(argv[i])-(eq+1);
        strncpy(tmpstr1,argv[i], eq);
        tmpstr1[eq]='\0';
        strncpy(tmpstr2,argv[i]+eq+1, sz);
        tmpstr2[sz]='\0';
        opt = tmpstr1; 
        val = tmpstr2; 
      }
      else
      {
        opt = argv[i];
        val = "";
      }
    }
    else
    {
      /** This is an option with only one character */
      if(!strncmp(argv[i],"-",1))
      {
        int s=0;
        char option[2];
        string long_opt;

        option[0] = argv[i][1];
        option[1] = '\0';

        argv[i]=argv[i]+2;
        long_opt = option;

        val = argv[i];
        opt = look_for_option_name(long_opt);

        if(opt == "")
        {
          cout << "Error: unrecognized short option '-" << long_opt << "'" << endl;
          return false;
        }
      }
      else
      {
        opt = "";
        val = argv[i];
      }
    }

    if(opt!="")
    {
      if(!options.count(opt))
      {
        cout << "Error: unrecognized option '" << opt << "'" << endl;
        return false;
      }
      else
      {
        if(val == "" && options[opt].type == OPTION_MULTIPLE_ARG)
        {
          if(i<(argc-1))
          {
            options[opt].values.insert(argv[i+1]);
            i++;
          }
          else
          {
            cout << "Error: option '" << opt << "' needs an extra parameter" << endl;
            return false;
          }
        }
        else
        {
          options[opt].values.insert(val);
        }
      }
    }
    else
    {
      parameters.push_back(val); 
    }
  }	
  return true;
}

#define TABS 30

void arg_parser::print_help()
{
  cout << "Usage: " << prog_name << " [options] " << endl;
  cout << endl;

  cout << "Mandatory arguments to long options are mandatory for short options too." << endl;

  option_list::iterator i;
  for(i = options.begin();
      i != options.end();
      i++)
  {
    unsigned int size ; 
    cout << "  -" << i->second.short_option_name.c_str()[0] << ", --" << i->second.option_name;
    size = strlen(i->second.option_name.c_str());
    if(i->second.type != OPTION_SIMPLE)
    {
      cout << "=" << i->second.argument_name;
      size += 1 + strlen(i->second.argument_name.c_str());
    }

    for(int j=size; j<=TABS; j++)
      cout << " ";
    cout << i->second.comment << endl;
  }
  cout << endl << endl;
  cout << bug_report << endl;
}

void arg_parser::print_short_help()
{
  int n=0;

  option_list::iterator i;
  cout << prog_name << ": ";
  for(i = options.begin();
      i != options.end();
      i++)
  {
    unsigned int size ; 
    cout << "[ -" << i->second.short_option_name.c_str()[0] << " | --" << i->second.option_name;
    size = strlen(i->second.option_name.c_str());
    if(i->second.type != OPTION_SIMPLE)
    {
      cout << "=" << i->second.argument_name;
      size += 1 + strlen(i->second.argument_name.c_str());
    }
    cout << "] ";

    n++;

    if(!(n % 3)) 
    {
      cout << endl;
      for(int k=0; k<strlen(prog_name.c_str()); k++)
        cout << " ";
      cout << "  ";

    }
  }
  cout << parameter_name; 
  cout << endl;
}

extern "C"
{

void xml_m3_declare_parameter(arg_parser *p, string name)
{
    p->insert(name.c_str(), name.c_str()[0], "ARG", "No man.", OPTION_MULTIPLE_ARG);
}

bool xml_m3_process_options(arg_parser *p, int argc, char **argv)
{
        if(!p->process_options(argc, argv))
            return false;
        else
            return true;
  
}

int  xml_m3_get_integer(arg_parser *current_arg_parser, string name)
{
    if(current_arg_parser->option_is_set(name))
    {   
        set<string>::iterator i;
        i = current_arg_parser->options[name].values.begin();

        if(i!=current_arg_parser->options[name].values.end())
        {
            return atoi(i->c_str());
        }
        else
            return 0;
    }
}

string xml_m3_get_string(arg_parser *current_arg_parser, string name)
{
    if(current_arg_parser->option_is_set(name))
    {   
        set<string>::iterator i;
        i = current_arg_parser->options[name].values.begin();

        if(i!=current_arg_parser->options[name].values.end())
        {
            return (*i);
        }
        else
            return "";
    }
}

/** Missing masks and permutations. TBD */

bool xml_m3_is_parameter_valid(arg_parser *current_arg_parser, string name)
{
    if(current_arg_parser->option_is_set(name))
    {
        set<string>::iterator i;
        i = current_arg_parser->options[name].values.begin();

        if(i!=current_arg_parser->options[name].values.end())
        {  
            return true;
        }
    }
    return false;
}

void xml_m3_emit_metric(string name, double value)
{
    cout << "@" << name << "=" << value << "@" << endl;
}

void xml_m3_emit_error(string error)
{
    cout << "@error=" << error << "@" << endl;
}


}
