%{
	#include "Types.hpp"
	#include "lex.yy.h"
	#include "parser_debug.hpp"

	extern int yylex();
	extern int yyparse();
	void yyerror(const char *);

	static Rtsp::Request *req; // Not null, guaranteed
%}

%union {
	int   ival;
	float fval;
	char  *sval;
}

%token <ival> INT
%token <fval> FLOAT
%token <sval> STRING

%token NL


%%


main:
	STRING
	;


%%


// WARN: it must be single-threaded
void parse(Rtsp::Request &request, Rtsp::Data data)
{
	debug("Parse started");
	req = &request;
	yyin = nullptr;
	// yy_scan_buffer(reinterpret_cast<char *>(data.data), data.len);
	auto bufState = yy_scan_bytes(reinterpret_cast<char *>(data.data), data.len);
	yyparse();
	yy_delete_buffer(bufState);
}

#if DEBUG_FLEX != 1

void yyerror(const char *s)
{
	debug("ERROR:");
	debug(s);
}

#endif