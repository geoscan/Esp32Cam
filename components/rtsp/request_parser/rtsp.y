%{
	extern int yylex();
	extern int yyparse();
%}

%union {
	int   ival;
	float fval;
	char  *sval;
}

%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING

%token SLASH
%token NL
%token OPT_SPACES
%token COL
%token EQ


%%


lines:
	  lines single_line
	| single_line
	;


single_line:
	single_line_content NL
	;


single_line_content:
	| any_strings
	;


// This parser is not intended to verify user request,
// but to extract information from it. So we just
// neglect any line we don't need.
any_strings:
	  any_strings STRING
	|
	;


%%