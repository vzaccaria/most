/* scanner for a toy Pascal-like language */

%{
/* need this for the call to atof() below */
%}

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


%%

"#<"    {
	    putchar('%');
	}  

">#"    {
	    putchar('%');
	}  

"("    {
	    printf("begin_map");
	}  

")#"    {
	    printf("end_map");
	}  
%%

int main( int argc, char **argv )
{
	++argv, --argc;  /* skip over program name */
	if ( argc > 0 )
		yyin = fopen( argv[0], "r" );
	else
		yyin = stdin;

	yylex();
}

