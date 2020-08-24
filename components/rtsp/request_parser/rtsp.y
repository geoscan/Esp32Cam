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
	int   uival;
	float fval;
}

%token <uival> UINT
%token <fval> FLOAT

%token CSEQ
%token CLIENT_PORT
%token SESSION
%token UDP

%%


text:
	word
|	text word	
|	text cseq
|	text transport
|	text session
;

 
word:
	UINT
|	FLOAT
;



cseq: 
	CSEQ UINT
	{
		debug("GOT: CSeq");
		debug($2);
		req->cseq = $2;
	}
;


transport:
	CLIENT_PORT UINT
	{
		debug("GOT: client_port");
		debug($2);
		req->clientPort = $2;
	}
|	UDP
	{
		debug("GOT: Transport: ... UDP");
		req->udp = true;
	}
;


session:
	SESSION UINT
	{
		debug("GOT: Session");
		debug($2);
		req->session = $2;
	}


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