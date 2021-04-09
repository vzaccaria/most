%{
#include <st_object.h>
#include <st_point.h>
#include <st_list.h>
#include <st_map.h>
#include <st_shell_variables.h>
#include <st_sim_utils.h>
#include <st_ast.h>
#define YYSTYPE st_ast *
#include <math.h>
#include <iostream>
#include <memory> 
#include <st_parser.h>
#include <st_commands.h>
#include <st_driver_utils.h>

void yyerror(const char *);
int yylex();

extern const char *current_file_read;

%}

%token NUM
%token STRING
%token IDENTIFIER
%token VARIABLE_VALUE
%token DB_INSERT_POINT
%token SET_OBJECTIVE
%token SET_FUNCTION
%token SET_CONSTRAINT
%token SET_PROCEDURE
%token SET_LOCAL
%token SET_METRIC
%token RETURN
%token FUNCTIONAL
%token SET 
%token MECHO
%token OBJ_REPR
%token ENDOFFILE
%token FLOAT
%token WHILE
%token DONE
%token ENDIF
%token FOREACH 
%token IN 
%token IF 
%token ELSE 
%token EQEQ
%token LEQ
%token GEQ
%token NEQ
%token MAP_BEGIN
%token MAP_END
%token MNSMNS
%token APPEND
%token CREATE
%token ANON

%left '<' '>' EQEQ NEQ LEQ GEQ
%left '-' '+' '|'
%left '*' '/' '&'
%left '@'
%left UMINUS
%right '!'
%right '^'
%left ';'  
%left ELSE


%% /* Grammar rules and actions follow */

file: input ENDOFFILE
      {
	   *prs_current_parser_session_return_ast() = $1;
	   YYACCEPT; 
      }

input:  /* empty */     { 
     				if(!prs_current_parser_session_is_interactive())
				{
					$$ = (new st_ast_compound_commands());
				}
     			
			}
        | input line { 
		       if(prs_current_parser_session_is_interactive())
		       {
			       try
			       {
				  st_ast_expression_stack stack;
				  st_object *res = st_ast_eval($2, &stack, &current_environment); 
				  if($2)
				     delete $2;
				  st_object_discard(res);
				  int location_trigger = @1.first_line;
			       }
			       catch(exception& e)
			       {
				  prs_display_error(e.what());
			       }
			}
			else
			{
				if($2 != NULL)
				{
					to<st_ast_compound_commands *>($1)->compound_commands.push_back($2);
					$1->line = @2.first_line;
					$1->decl_file_name = current_file_read;
				}
                                $$=$1;
			}
		     }
        ;

line:	
   	 '\n' 					{ $$ = NULL; }
    	| command_list 	 '\n'			{ $$ = $1; }
					
	;

/* Commands on the same line can be given separated by ';' */
command_list: command				{ 
	    					  $$ = new st_ast_compound_commands();
	    					  to<st_ast_compound_commands *>($$)->compound_commands.push_front($1);
					  	  $$->line = @1.first_line;
						  $$->decl_file_name = current_file_read;
						}
	    | command_list ';' command  	{ 
						 to<st_ast_compound_commands *>($1)->compound_commands.push_back($3);
					  	 $1->line = @2.first_line;
						 $1->decl_file_name = current_file_read;
						 $$=$1;
						}
	    ;

command: 
	 IDENTIFIER command_opts { 
						  st_ast_simple_command *ast = new st_ast_simple_command();
					  	  ast->line = @1.first_line;
						  ast->decl_file_name = current_file_read;
						  ast->command_name = to<st_ast_identifier *>($1)->id_name;
						  st_copy_ast_map(to<st_ast_construct_map *>($2)->map_elements, ast->command_parameters); 
						  delete $1;
						  delete $2;
						  $$ = ast;
						}	
	| object_expression
        | SET IDENTIFIER '=' object_expression { 
						  st_ast_set *ast = new st_ast_set();
						  ast->variable_name = to<st_ast_identifier *>($2)->id_name; 
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  ast->expression = $4;
						  delete $2;
						  ast->local=false;
						  $$ = (ast);
						}
        | SET_LOCAL IDENTIFIER '=' object_expression { 
						  st_ast_set *ast = new st_ast_set();
						  ast->variable_name = to<st_ast_identifier *>($2)->id_name; 
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  ast->expression = $4;
						  delete $2;
						  ast->local=true;
						  $$ = (ast);
						}
        | SET IDENTIFIER '@' expr '=' object_expression { 
						  st_ast_set *ast = new st_ast_set();
						  ast->variable_name = to<st_ast_identifier *>($2)->id_name; 
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  ast->expression = $6;
						  ast->at = true;
						  ast->index = $4;
						  delete $2;
						  ast->local=false;
						  $$ = (ast);
						}
        | SET_LOCAL IDENTIFIER '@' expr '=' object_expression { 
						  st_ast_set *ast = new st_ast_set();
						  ast->variable_name = to<st_ast_identifier *>($2)->id_name; 
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  ast->expression = $6;
						  ast->at = true;
						  ast->index = $4;
						  delete $2;
						  ast->local=true;
						  $$ = (ast);
						}
        | SET_OBJECTIVE IDENTIFIER '(' IDENTIFIER ')' '=' object_expression { 
						  st_ast_define_objective *ast = new st_ast_define_objective();
						  ast->name= to<st_ast_identifier *>($2)->id_name; 
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  delete $2;
						  ast->point_name = to<st_ast_identifier *>($4)->id_name;
						  delete $4;
						  ast->objective_expression = $7;
						  $$ = (ast);
						}
        | SET_METRIC IDENTIFIER '(' IDENTIFIER ')' '=' object_expression { 
						  st_ast_define_metric *ast = new st_ast_define_metric();
						  ast->name= to<st_ast_identifier *>($2)->id_name; 
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  delete $2;
						  ast->point_name = to<st_ast_identifier *>($4)->id_name;
						  delete $4;
						  ast->metric_expression = $7;
						  $$ = (ast);
						}
        | SET_CONSTRAINT IDENTIFIER '(' IDENTIFIER ')' '=' object_expression { 
						  st_ast_define_constraint *ast = new st_ast_define_constraint();
						  ast->name= to<st_ast_identifier *>($2)->id_name; 
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  delete $2;
						  ast->point_name = to<st_ast_identifier *>($4)->id_name;
						  delete $4;
						  ast->constraint_expression = $7;
						  $$ = (ast);
						}
        | SET_FUNCTION IDENTIFIER '(' id_list ')' '=' object_expression { 
						  st_ast_define_function *ast = new st_ast_define_function();
						  ast->name= to<st_ast_identifier *>($2)->id_name; 
						  ast->local_parameters = $4;
						  ast->function_expression = $7;
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  delete $2;
						  $$ = (ast);
						}

	| SET_PROCEDURE IDENTIFIER '(' id_list ')' compound_statements DONE
						{
						   st_ast_define_procedure *ast = new st_ast_define_procedure();
						   ast->name = to<st_ast_identifier *>($2)->id_name;
					  	   ast->line = @2.first_line;
						   ast->decl_file_name = current_file_read;
						   delete $2;
						   ast->local_parameters = $4;
						   ast->compound_commands = $6;
						   $$ = (ast);
						}
        | MECHO object_expression 		{ 
						  st_ast_echo *ast = new st_ast_echo();
						  ast->expression = $2;
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  $$ = (ast);
						}

        | MECHO object_expression APPEND STRING	{ 
						  st_ast_echo *ast = new st_ast_echo();
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  ast->expression = $2;
						  ast->mode = "append";
						  ast->file_name = to<st_string *>(to<st_ast_leaf *>($4)->leaf_value)->get_string();
						  delete $4;
						  $$ = (ast);
						}
        | MECHO object_expression CREATE STRING	{ 
						  st_ast_echo *ast = new st_ast_echo();
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  ast->expression = $2;
						  ast->mode = "create";
						  ast->file_name = to<st_string *>(to<st_ast_leaf *>($4)->leaf_value)->get_string();
						  delete $4;
						  $$ = (ast);
						}
	| WHILE expr compound_statements DONE 
						{
						  st_ast_command_while *ast = new st_ast_command_while();
						  ast->evaluation_expression = $2;
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  st_copy_ast_list(to<st_ast_compound_commands*>($3)->compound_commands,ast->compound_commands);
						  delete $3;
						  $$ = (ast);	
						}
	| FOREACH IDENTIFIER IN object_expression compound_statements DONE
						{
						  st_ast_command_for *ast = new st_ast_command_for();
						  ast->iterator_variable = to<st_ast_identifier *>($2)->id_name;
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  ast->list_of_values = $4;
						  st_copy_ast_list(to<st_ast_compound_commands*>($5)->compound_commands,ast->compound_commands);
						  delete $5;
						  delete $2;
						  $$ = (ast);
						}

	| IF expr compound_statements else_stmnt ENDIF
						{
						  st_ast_command_if *ast = new st_ast_command_if();
						  ast->evaluation_expression = $2;
					  	  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  st_copy_ast_list(to<st_ast_compound_commands*>($3)->compound_commands,
							ast->then_commands);
						  delete $3;
						  if($4)
						  { 
							st_copy_ast_list(to<st_ast_compound_commands*>($4)->compound_commands,
								ast->else_commands);
							delete $4;
						  }
						  $$ = (ast);	
						}

	;

else_stmnt: 			     { $$=NULL; }
	  | ELSE compound_statements { $$=$2; } 
	  ;

newline_list: '\n'
	      | newline_list '\n'
	      ;

/** In a control structure such as if, while, foreach etc.. If commands are on the same line they should be separated by a ';' */

compound_statements: /* empty */		{ $$ = (new st_ast_compound_commands()); }
		 | compound_statements newline_list
		 | compound_statements command newline_list
						{
						 to<st_ast_compound_commands *>($1)->compound_commands.push_back($2);
					  	 $1->line = @2.first_line;
						 $1->decl_file_name = current_file_read;
						 $$=$1;
						}
		 | compound_statements command ';'
						{ 
						 to<st_ast_compound_commands *>($1)->compound_commands.push_back($2); 
					  	 $1->line = @2.first_line;
						 $1->decl_file_name = current_file_read;
						 $$=$1;
					       }
		 ;

object_expression: 
	 point
	| full_object_expression
	| expr
	;

/* Integer numbers can be used in expressions also */
expr:	
    	'-' expr %prec UMINUS		{
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = NULL; ast->right = $2; ast->operator_name = "unary-";
					  ast->line = @2.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
	| '+' expr			{
					  $$ = $2; 
					}
    
	| ANON IDENTIFIER {
						  st_ast_define_anon_function *ast  = new st_ast_define_anon_function();
						  ast->existing = true;
						  ast->existing_name = to<st_ast_identifier *>($2)->id_name;
						  ast->function_expression = NULL;
						  ast->line = @2.first_line;
						  ast->decl_file_name = current_file_read;
						  delete $2;
						  $$ = (ast);
						}

        | ANON '(' id_list ')' '=' object_expression { 
						  st_ast_define_anon_function *ast = new st_ast_define_anon_function();
						  ast->existing = false;
						  ast->existing_name= "nil";
						  ast->local_parameters = $3;
						  ast->function_expression = $6;
					  	  ast->line = @3.first_line;
						  ast->decl_file_name = current_file_read;
						  $$ = (ast);
						}


	| expr '+' expr			
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "+";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}

    	| expr '-' expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "-";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
    	| expr '*' expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "*";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
    	| expr '/' expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "/";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
	| expr '<' expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "<";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
	| expr '>' expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = ">";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
	| expr EQEQ expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "==";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
	| expr LEQ expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "<=";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
	| expr GEQ expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = ">=";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
	| expr NEQ expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "!=";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
        | expr '|' expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "|";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
        | expr '&' expr
    					{ 
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "&";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
	| '(' expr ')'
    					{ 
					  $$ = $2;
					}
	| IDENTIFIER '(' vect_objs ')'
    					{ 
					  st_ast_function_call *ast = new st_ast_function_call(); 
    					  ast->operand = $3; ast->function_name = to<st_ast_identifier *>($1)->id_name;;
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  ast->stack_level = -1;
					  delete $1;
					  $$ = (ast); 
					}
	| expr '(' vect_objs ')'        { 
                                          st_ast_anon_function_call *ast = new st_ast_anon_function_call();
                                          ast->operand = $3; 
					  ast->function_name = ($1);;
                                          ast->line = @1.first_line;
                                          ast->decl_file_name = current_file_read;
                                          $$ = (ast);
                                        }
	| FUNCTIONAL '(' IDENTIFIER ',' expr ',' expr ',' expr ')'
					{
					   st_ast_functional *ast = new st_ast_functional();
					   ast->functional_name = to<st_ast_identifier *>($1)->id_name;
					   ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					   delete $1;
					   ast->iterator = to<st_ast_identifier *>($3)->id_name;
					   delete $3;
					   ast->min = $5;
					   ast->max = $7;
					   ast->expression = $9;
					   $$ = ast;
					}
	| '!' expr			{
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = NULL; ast->right = $2; ast->operator_name = "!";
					  ast->line = @2.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
	| expr '@' expr			{
					  st_ast_expression *ast = new st_ast_expression(); 
    					  ast->left = $1; ast->right = $3; ast->operator_name = "@";
					  ast->line = @1.first_line;
					  ast->decl_file_name = current_file_read;
					  $$ = (ast); 
					}
	| NUM
	| FLOAT
	| VARIABLE_VALUE
	| STRING
	| list
	| vect
	| map
	;
	
list: '{' list_objs '}'				{ $2->line = @1.first_line; $2->decl_file_name = current_file_read; $$=$2;}
        ;

list_objs: list_objs_e				{ $$=$1;}
	   | list_objs_e list_obj_ne		{ 
						  delete $1;
						  $$=$2;
						}
	   ;

list_objs_e: /* empty */ 			{ $$=(new st_ast_construct_list());}
	  ;

list_obj_ne: object_expression			{ 
	   					  st_ast_construct_list *astlist = new st_ast_construct_list(); 
						  astlist->list_elements.push_back($1);
						  astlist->line = @1.first_line;
						  astlist->decl_file_name = current_file_read;
						  $$=astlist;
	   					}
	  | list_obj_ne ',' object_expression   { to<st_ast_construct_list *>($1)->list_elements.push_back($3); $1->line=@3.first_line; $1->decl_file_name = current_file_read; $$ = $1;}

	;

point: '%' point_objs '%'     			{ $2->line = @1.first_line; $2->decl_file_name = current_file_read; $$=$2; }
	;

vect: '[' vect_objs ']'       			{ $2->line = @1.first_line; $2->decl_file_name = current_file_read; $$=$2; }
	;

vect_objs: vect_objs_e				{ $$=$1;}
	| vect_objs_e vect_objs_ne		{ 
						     delete $1;
                                                     $$=$2;
						}
	;

vect_objs_e: /* empty */                          { $$=(new st_ast_construct_vector());}
	   ;
vect_objs_ne: object_expression			{
	    					  st_ast_construct_vector *astlist = new st_ast_construct_vector();
                                                  astlist->vector_coordinates.push_back($1);
						  astlist->line = @1.first_line;
						  astlist->decl_file_name = current_file_read;
                                                  $$=astlist;
						}
	 | vect_objs_ne ',' object_expression   { to<st_ast_construct_vector *>($1)->vector_coordinates.push_back($3); $1->decl_file_name = current_file_read; $1->line=@3.first_line; $$ = $1;}
	 ;

id_list: id_list_e 				{ $$=$1;}
	| id_list_e id_list_ne		{ 
						  delete $1;
                                                  $$=$2;
						}
	;

id_list_e: /* empty */                          { $$=(new st_ast_construct_vector()); }
	   ;
id_list_ne: IDENTIFIER {
	    					  st_ast_construct_vector *astlist = new st_ast_construct_vector();
                                                  astlist->vector_coordinates.push_back($1);
                                                  $$=astlist;
						  $$->line = @1.first_line;
						  $$->decl_file_name = current_file_read;
						}
	 | id_list_ne ',' IDENTIFIER { to<st_ast_construct_vector *>($1)->vector_coordinates.push_back($3); $1->line=@3.first_line; $1->decl_file_name = current_file_read; $$ = $1;}
	 ;

point_objs: /* empty */    			{ $$=(new st_ast_construct_point());} 
	| point_objs NUM   			{ to<st_ast_construct_point *>($1)->point_coordinates.push_back($2); $1->line=@2.first_line; $1->decl_file_name = current_file_read; $$=$1;} 	
	| point_objs VARIABLE_VALUE		{ to<st_ast_construct_point *>($1)->point_coordinates.push_back($2); $1->line=@2.first_line; $1->decl_file_name = current_file_read; $$=$1;} 	
	;

full_object_expression:
	OBJ_REPR object_expression map  { 
						     $$=(new st_ast_full_object());
						     to<st_ast_full_object *>($$)->full_object_description = $2;
						     to<st_ast_full_object *>($$)->properties = $3;
						     $$->line = @2.first_line;
						     $$->decl_file_name = current_file_read;
						   }
	;

map: MAP_BEGIN map_elements MAP_END 			{ $2->line = @1.first_line; $2->decl_file_name = current_file_read; $$=$2; }
      ;

/* We use IDENTIFIER here cause it is interpreted as an st_string */
map_elements: /* empty */ 			{ $$=(new st_ast_construct_map());}
	| map_elements IDENTIFIER '=' object_expression 	
						{ 
						   string name = to<st_ast_identifier *>($2)->id_name;
						   delete $2;
						   pair<string, st_ast *> p(name, $4);
						   to<st_ast_construct_map *>($1)->map_elements.insert(p);
						   $1->line = @2.first_line;
						   $1->decl_file_name = current_file_read;
						   $$=$1;
						}
	;

command_opts: /* empty */ 			{ $$=(new st_ast_construct_map()); }
	    | command_opts MNSMNS IDENTIFIER '=' command_opt_value {
							string name = to<st_ast_identifier *>($3)->id_name; 
							delete $3;
						 	pair<string, st_ast *> p(name, $5);
							to<st_ast_construct_map *>($1)->map_elements.insert(p);
							$1->line = @3.first_line;
						        $1->decl_file_name = current_file_read;
							$$=$1;
						      }	
	    | command_opts command_opt_value	{
							pair<string, st_ast *> p("", $2);
							to<st_ast_construct_map *>($1)->map_elements.insert(p);
							$1->line = @2.first_line;
						        $1->decl_file_name = current_file_read;
							$$=$1;
						}
	    ;

command_opt_value:
 	object_expression	
	| IDENTIFIER
	| SET					{ /** When using set in an help environment, threat it differently */
						  st_ast_identifier *new_leaf = 
						    to<st_ast_identifier *>((new st_ast_identifier()));
                                        	  new_leaf->id_name = "set"; 
					          new_leaf->line = @1.first_line;
						  new_leaf->decl_file_name = current_file_read;
						  $$=new_leaf;
						}
						  
	| SET_OBJECTIVE					{ /** When using set in an help environment, threat it differently */
						  st_ast_identifier *new_leaf = 
						    to<st_ast_identifier *>((new st_ast_identifier()));
                                        	  new_leaf->id_name = "set_objective"; 
					          new_leaf->line = @1.first_line;
						  new_leaf->decl_file_name = current_file_read;
						  $$=new_leaf;
						}
	| SET_FUNCTION				{ /** When using set in an help environment, threat it differently */
						  st_ast_identifier *new_leaf = 
						    to<st_ast_identifier *>((new st_ast_identifier()));
                                        	  new_leaf->id_name = "set_function"; 
					          new_leaf->line = @1.first_line;
						  new_leaf->decl_file_name = current_file_read;
						  $$=new_leaf;
						}
	| SET_METRIC				{ /** When using set in an help environment, threat it differently */
						  st_ast_identifier *new_leaf = 
						    to<st_ast_identifier *>((new st_ast_identifier()));
                                        	  new_leaf->id_name = "set_metric"; 
					          new_leaf->line = @1.first_line;
						  new_leaf->decl_file_name = current_file_read;
						  $$=new_leaf;
						}
	| SET_CONSTRAINT			{ /** When using set in an help environment, threat it differently */
						  st_ast_identifier *new_leaf = 
						    to<st_ast_identifier *>((new st_ast_identifier()));
                                        	  new_leaf->id_name = "set_constraint"; 
					          new_leaf->line = @1.first_line;
						  new_leaf->decl_file_name = current_file_read;
						  $$=new_leaf;
						}
	;

%%
