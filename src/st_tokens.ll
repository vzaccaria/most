%{
#include <memory>
#include <sstream>
#include "st_object.h"
#include "st_ast.h"
#define YYSTYPE st_ast *
#include "st_grammar.hh"
#include "st_parser.h"
#include "st_commands.h"

extern YYLTYPE yylloc;

const char *current_file_read;
%}
%option noyywrap

DIGIT    [0-9]+
FLOAT	 [0-9]+[\.][0-9]*
FLOAT_S  [0-9]+[eE][\+\-]?[0-9]*
FLOAT_S2 [0-9]+[\.][0-9]*[eE][\+\-]?[0-9]*
ID       [A-Za-z\_\?][A-Za-z0-9\_\.\:]*
NL	 ([\n])
WS       ([ \t\r])
IDS      ([^\"]*)
COM	 [\#]([^\n]*) 
TRUE	 [Tt][Rr][Uu][Ee]
FALSE	 [Ff][Aa][Ll][Ss][Ee]

%x incl


%{

#if !defined(__BSD_READLINE__)
    #include <readline/readline.h>
    #include <readline/history.h>
#else
    #if defined(__MAC_OSX__) 
        #include <readline/readline.h>
    	#include <readline/history.h>
    #else
	#if defined(__RHEL__)
        	#include <editline/readline.h>
	#else
        	#include <editline/readline.h>
		#include <editline/history.h>
	#endif
    #endif /* ! MAC_OS_X */
#endif /* ! READLINE */

#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
 
#include <string.h>
/* Use the readline library to read the input. */
void yyerror(const char *s);
#ifdef YY_INPUT
#undef YY_INPUT
#endif /* YY_INPUT */
extern FILE *static_log;

#define ST_PROMPT "most> "

#define YY_INPUT(buf, result, max_size) \
	if (YY_CURRENT_BUFFER->yy_is_interactive) \
	{ \
		size_t	n = 0; \
		char	*readline_in = NULL; \
		\
		readline_in = readline(ST_PROMPT); \
		if ((NULL == readline_in) && ferror(yyin)) \
		{ \
			YY_FATAL_ERROR("Input failed"); \
		} \
		else if (NULL == readline_in) \
		{ \
			YY_FATAL_ERROR("Dont know howto handle"); \
		} \
		\
		if(strlen(readline_in)!=0) { \
			add_history(readline_in); \
			if(static_log) \
			   fwrite(readline_in, strlen(readline_in), 1, static_log);\
			   fwrite("\n", strlen("\n"), 1, static_log);\
		}\
		\
		for (n = 0; (NULL != readline_in) && (n < max_size) && ('\0' != *readline_in); ++n, ++readline_in) \
			buf[n] = (char)*readline_in; \
		/* flex expects the string to be terminated 
		 * by a newline character. */ \
		buf[n++] = '\n'; \
		result = n; \
	} \
	else { \
		/* This is the default behavior as provided by flex */ \
		errno = 0; \
		while (((result = fread(buf, 1, max_size, yyin)) == 0) && (ferror(yyin))) \
		{ \
			if (errno != EINTR) \
			{ \
				YY_FATAL_ERROR("Read from disk failed"); \
				break; \
			} \
			errno = 0; \
			clearerr(yyin); \
		} \
	} \

%}


%%

<<EOF>>				{ return ENDOFFILE;}
{DIGIT}				{ 
					st_ast_leaf *new_leaf = to<st_ast_leaf *>((new st_ast_leaf()));
					new_leaf->leaf_value = new st_integer(atoi(yytext));
					new_leaf->line = yylloc.first_line;
					new_leaf->decl_file_name = current_file_read;
					yylval = new_leaf; return NUM; 
				}
"inf"|"nan"|{FLOAT}|{FLOAT_S}|{FLOAT_S2}	{ 
					st_ast_leaf *new_leaf = to<st_ast_leaf *>((new st_ast_leaf()));
					new_leaf->leaf_value = new st_double(atof(yytext));
					new_leaf->line = yylloc.first_line;
					new_leaf->decl_file_name = current_file_read;
					yylval = new_leaf; return FLOAT; 
				}
{TRUE}				{
					st_ast_leaf *new_leaf = to<st_ast_leaf *>((new st_ast_leaf()));
					new_leaf->leaf_value = new st_integer(1);
					new_leaf->line = yylloc.first_line;
					new_leaf->decl_file_name = current_file_read;
					yylval = new_leaf; return NUM; 
				}
{FALSE}				{
					st_ast_leaf *new_leaf = to<st_ast_leaf *>((new st_ast_leaf()));
					new_leaf->leaf_value = new st_integer(0);
					new_leaf->line = yylloc.first_line;
					new_leaf->decl_file_name = current_file_read;
					yylval = new_leaf; return NUM; 
				}
{NL}				{ yylloc.first_line += 1; return '\n'; }
{WS}+	 			{}	
{COM}				{}
"_obj_repr_"			{ return OBJ_REPR; }
"=="				{ return EQEQ; }
">="				{ return GEQ; }
"<="				{ return LEQ; }
"!="				{ return NEQ; }
">>"				{ return APPEND; }
"!>"				{ return CREATE; }
"--"				{ return MNSMNS; }
"(*"				{ return MAP_BEGIN; }
"*)"				{ return MAP_END; }
"("|")"|"{"|"}"|"<"|">"|"="|"+"|"-"|"*"|"/"|"["|"]"|"@"|"#"|"$"|";"|"%"|"!"|"\'"|","|"&"|"|" { return *yytext;}
"^"				{ return ANON; }
"set_objective"			{ return SET_OBJECTIVE;}
"set_function"			{ return SET_FUNCTION;}
"set_constraint"		{ return SET_CONSTRAINT;}
"set_procedure"			{ return SET_PROCEDURE;}
"set_local"                     { return SET_LOCAL;}
"set_metric"			{ return SET_METRIC;}
"set"                           { return SET;}
"echo"				{ return MECHO; }
"while"                         { return WHILE;}
"foreach" 			{ return FOREACH;}
"in" 				{ return IN;}
"if"                         	{ return IF;}
"else"                         	{ return ELSE;}
"done"                         	{ return DONE;}
"endif"                         { return ENDIF;}
"max"|"min"|"sum"|"prod"|"avg"|"geomavg"	{
					st_ast_identifier *new_leaf = to<st_ast_identifier *>((new st_ast_identifier()));
					new_leaf->id_name = string(yytext);
					new_leaf->line = yylloc.first_line;
					new_leaf->decl_file_name = current_file_read;
					yylval = new_leaf; return FUNCTIONAL;
				}
${ID}				{ 
					st_ast_variable *new_leaf = to<st_ast_variable *>((new st_ast_variable()));
					new_leaf->variable_name = string(yytext+1);
					new_leaf->line = yylloc.first_line;
					new_leaf->decl_file_name = current_file_read;
					yylval = new_leaf; return VARIABLE_VALUE;
				}
\"{IDS}\"			{ 
					yytext[strlen(yytext)-1]='\0'; 
					st_ast_leaf *new_leaf = to<st_ast_leaf *>((new st_ast_leaf()));
					new_leaf->leaf_value = new st_string(yytext+1);
					new_leaf->line = yylloc.first_line;
					new_leaf->decl_file_name = current_file_read;
					yylval = new_leaf; return STRING;
				}
"/*"([^\*]|\*[^/])*"*/" { 
					yytext[strlen(yytext)-2]='\0'; 
					st_ast_leaf_R *new_leaf = to<st_ast_leaf_R *>((new st_ast_leaf_R()));
					new_leaf->leaf_value = (yytext+2);
					new_leaf->line = yylloc.first_line;
					new_leaf->decl_file_name = current_file_read;
					yylval = new_leaf; return STRING;
				}
{ID}				{ 
					st_ast_identifier *new_leaf = to<st_ast_identifier *>((new st_ast_identifier()));
					new_leaf->id_name = string(yytext);
					new_leaf->line = yylloc.first_line;
					new_leaf->decl_file_name = current_file_read;
					yylval = new_leaf; return IDENTIFIER;
				}
.				{ yyerror((string("Unrecognized token ")+yytext).c_str()); }
%%

#define prs_get_current_parser_session() (sessions[sessions.size()-1])

struct current_parser_session
{
	bool is_interactive;
	string file_name;
	st_ast *parsed_ast;
	FILE *current_input_channel;
};

vector<current_parser_session> sessions;


void yyerror(const char *s)
{
     char buf[40];
     if(!prs_get_current_parser_session().is_interactive)
     {
	     sprintf(buf, "%d", yylloc.first_line);
	     string str = string("")+buf+", "+s;
	     throw std::logic_error(str.c_str());
     }
     else
     {
	     throw std::logic_error(s); 
     }
}


int mpi_max_parseable_size = YY_READ_BUF_SIZE;


bool prs_current_parser_session_is_interactive()
{
 	return (sessions[sessions.size()-1]).is_interactive;
}

st_ast **prs_current_parser_session_return_ast()
{
	return &((sessions[sessions.size()-1]).parsed_ast);
}


extern st_ast *last_known_ast;

vector<string> files_read;

string prs_get_last_line()
{
	int line = 0;
	if(last_known_ast)
		line = last_known_ast->line; 
	ostringstream linestream;
        linestream << line;
	return linestream.str();
}

string prs_get_last_file()
{
	if(last_known_ast)
	{
	  if(last_known_ast->decl_file_name)
		return last_known_ast->decl_file_name;
	}
	return "";
}

bool prs_get_last_file_interactive()
{
	if(last_known_ast)
	{
          if(!last_known_ast->decl_file_name)
	  {
		return true;
	  }
	  else
	  {
	  	return false;	
	  }
	}
	return true; 
}


bool prs_go_interactive()
{
    current_parser_session s;
    s.is_interactive = true;
    sessions.push_back(s);
    bool error = false;
    yyrestart(stdin);
    bool exit = false;
    while(!exit)
    {
	    try
	    {    
		last_known_ast = NULL;
		current_file_read = NULL;
		yyparse();
	    }
	    catch(st_ast_return_exception &e)
	    {
		exit=true;
	    }
	    catch(exception& e)
	    {
		    string error = e.what();
		    prs_display_error(error);
	    }
    }
    sessions.resize(sessions.size()-1);
    return !error; 
}

#include "st_env.h"

extern st_env current_environment;
bool prs_parse_and_execute_file(const char *file_name)
{
    current_parser_session s;
    FILE *fh = fopen(file_name, "r");
    if(!fh)
	return false;
    s.current_input_channel = fh;
    s.is_interactive = false;
    s.file_name = file_name;

    files_read.push_back(file_name);    

    sessions.push_back(s);
    yylloc.first_line = 1;

    yyrestart(fh);
    bool error = false;

    try
    {    
	current_file_read = files_read[files_read.size()-1].c_str();
    	yyparse();
    }
    catch(exception& e)
    {
	    error = true;
	    prs_display_error_plain("file '"+prs_get_current_parser_session().file_name+"', line "+e.what());
    }
    if(!error)
    {
    	try
	{
		st_ast_expression_stack stack;
		last_known_ast = NULL;
		st_object *res = st_ast_eval(prs_get_current_parser_session().parsed_ast, &stack, &current_environment);
		st_object_discard(res);
		delete prs_get_current_parser_session().parsed_ast;
	}
	catch(exception& e)
	{
	        prs_display_error(e.what());	
		delete prs_get_current_parser_session().parsed_ast;
		/** Should find a way to delete the incomplete ast built so far */
		error = true;
	}
	catch(st_ast_return_exception& e)
	{	
		delete prs_get_current_parser_session().parsed_ast;
	}
    }
    sessions.resize(sessions.size()-1);
    fclose(s.current_input_channel);
    return !error; 
} 
