%{
	#include <cstdio>
	#include "rtsp.tab.h"
	#include "parser_debug.hpp"
	#include "Types.hpp"

	#if DEBUG_FLEX == 1
	#include <iostream>
	extern FILE *yyin;
	#else
	# define debug(...) /* Suppress output */
	#endif
	#define newtoken(__TOKEN__) debug("==========> @TOKEN@:"); debug(#__TOKEN__); return __TOKEN__

%}

%option noyywrap


%%
CSeq        {newtoken(CSEQ); }
client_port {newtoken(CLIENT_PORT); }
Session     {newtoken(SESSION); }
UDP         {newtoken(UDP); }
DESCRIBE    {newtoken(DESCRIBE);}
SETUP       {newtoken(SETUP);}
TEARDOWN    {newtoken(TEARDOWN);}
PLAY        {newtoken(PLAY);}
PAUSE       {newtoken(PAUSE);}


[\n\r]+           ;
\-?[0-9]+\.[0-9]+ {debug("==========> FLOAT"); 
                   debug(yytext); 
				   yylval.fval = atof(yytext); 
				   return FLOAT;
				  }
[0-9]+            {debug("==========> UINT"); 
                   debug(yytext); 
				   yylval.uival = atoi(yytext); 
				   return UINT; 
				  }
[ \t]             ;
.                 ;
%%

// see parser_debug.hpp
#if DEBUG_FLEX == 1 && PARSER_DEBUG == 1

int main(int, char**) {
	char buf[] = DEBUG_TEXT;

	auto bufState = yy_scan_bytes(reinterpret_cast<char *>(buf), sizeof(buf));

	// Parse through the input:
	while(yylex());

	yy_delete_buffer(bufState);
	
}

void yyerror(const char *s) {
	using namespace std;
	cout << "FLEX, parse error!  Message: " << s << endl;
	// might as well halt now:
	exit(-1);
}

#endif // DEBUG_FLEX == 1 && ...