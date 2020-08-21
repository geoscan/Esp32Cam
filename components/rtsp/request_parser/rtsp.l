%{
	#include rtsp.tab.h
%}

%option noyywrap

NL     \n|\r|(\r\n)
SPACES [ ]
STRING [^ \t\r\n]+
%%

\/             {return SLASH;}
{NL}           {return NL;}
{SPACES}*      {return OPT_SPACES;}
[ \t]          ;
:              {return COL;}
=              {return EQ;}
[0-9]+         {yylval.ival = atoi(yytext); return INT;}
[0-9]+\.[0-9]+ {yylval.fval = atof(yytext); return FLOAT;}
{STRING}       {return STRING;}

%%