%{
	#include <cstdio>
	#include "rtsp.tab.hpp"
	#include "parser_debug.hpp"
	#include "Types.hpp"

	#if DEBUG_FLEX == 1
	#include <iostream>
	extern FILE *yyin;
	#else
	# define debug(...) /* Suppress output */
	#endif
	
	#define token(__TOKEN__) debug("==========> @TOKEN@:"); debug(#__TOKEN__); return __TOKEN__
	#define stoken(__TOKEN__) yylval.sval = strdup(yytext); token(__TOKEN__)
	#define uitoken(__TOKEN__) yylval.uival = atoi(yytext); token(__TOKEN__)

%}

%option noyywrap

%x rtsphost rtspport rtspresource

URL [a-zA-Z0-9\.]


%%
rtsp\:\/\/                         BEGIN(rtsphost);
<rtsphost>[^\/:]+/\:               debug(yytext); BEGIN(rtspport);     stoken(RTSP_HOST);
<rtsphost>[^\/:]+/\/               debug(yytext); BEGIN(rtspresource); stoken(RTSP_HOST);
<rtspport>[0-9]+/\/                debug(yytext); BEGIN(rtspresource); uitoken(RTSP_PORT);
<rtspresource>[^[:space:]]+        debug(yytext); BEGIN(INITIAL);      stoken(RTSP_RESOURCE);
<rtsphost,rtspport,rtspresource>\s BEGIN(INITIAL);
<rtsphost,rtspport,rtspresource>.  ;


CSeq:          token(CSEQ); 
client_port    token(CLIENT_PORT); 
Session:       token(SESSION); 
\/UDP          token(UDP); 
DESCRIBE       token(DESCRIBE);
SETUP          token(SETUP);
TEARDOWN       token(TEARDOWN);
PLAY           token(PLAY);
PAUSE          token(PAUSE);
OPTION         token(OPTION);
mjpeg          token(MJPEG);

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