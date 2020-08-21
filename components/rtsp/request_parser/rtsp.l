%{
	#include <cstdio>
	#include "rtsp.tab.h"
	#include "parser_debug.hpp"
	#include "Types.hpp"

	#if DEBUG_FLEX == 1
	#include <iostream>
	extern FILE *yyin;
	#endif

%}

%option noyywrap


%%


[\n\r]+         {debug("------------NL------------"); 
                 return NL;
				}
[0-9]+\.[0-9]+  {debug("------------FLOAT------------"); 
                 debug(yytext); 
				 yylval.fval = atof(yytext); 
				 return FLOAT;
				}
[0-9]+          {debug("-----------INT------------"); 
                 debug(yytext); 
				 yylval.ival = atoi(yytext); 
				 return INT; 
				}
[^ \t\n\r;:]+   {debug("------------STRING------------"); 
                 debug(yytext); 
				 yylval.sval = strdup(yytext); 
				 return STRING;
				}
\;              ;
[ \t]    ;
%%

// see parser_debug.hpp
#if DEBUG_FLEX == 1 && PARSER_DEBUG == 1

int main(int, char**) {
	using namespace std;
	debug("flex debug");
	// Open a file handle to a particular file:
	FILE *myfile = fopen("inputfile", "r");
	// Make sure it is valid:
	if (!myfile) {
		cout << "I can't open a.snazzle.file!" << endl;
		return -1;
	}
	// Set Flex to read from it instead of defaulting to STDIN:
	yyin = myfile;
	
	// Parse through the input:
	while(yylex());
	
}

void yyerror(const char *s) {
	using namespace std;
	cout << "FLEX, parse error!  Message: " << s << endl;
	// might as well halt now:
	exit(-1);
}

#endif // DEBUG_FLEX == 1 && ...