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


// Please use nonterminal prefixes regarding user request they describe.
// If the rule is an ad-hoc, or related to 2 or more requests use
// prefix "common".
single_line_content:
	| common_session
	| common_cseq
	| any_strings
	;


common_session:
	"Session:" INT
	{
		req->session = $2;
	}
	;


common_cseq:
	"CSeq:" INT
	{
		req->cseq = $2;
	}
	;


// This parser is not intended to verify user request,
// but to extract information from it. So we just
// neglect any line we don't need.
any_strings:
	any_strings STRING
	|
	;


%%


// WARN: it must be single-threaded
void parse(Rtsp::Request &request, Rtsp::Data data)
{
	debug("Parse started");
	req = &request;
	yy_scan_buffer(reinterpret_cast<char *>(data.data), data.len);
	yyparse();
}

#if DEBUG_FLEX != 1

void yyerror(const char *s)
{
	debug("ERROR:");
	debug(s);
}

#endif