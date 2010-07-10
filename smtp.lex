%{
#include "y.tab.h"
%}

%%
HELO		return HELOTOK;
helo		return HELOTOK;
"MAIL FROM"	return MAILTOK;
"mail from"	return MAILTOK;
"RCPT TO"	return RCPTTOK;
"rcpt to"	return RCPTTOK;
DATA		return DATATOK;
data		return DATATOK;
NOOP		return NOOPTOK;
noop		return NOOPTOK;
RSET		return RSETTOK;
rset		return RSETTOK;
QUIT		return QUITTOK;
quit		return QUITTOK;
\:		return COLON;
\<		return LT;
\>		return GT;
\n		return NL;
[a-zA-Z0-9._%-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}	yylval.string=strdup(yytext);return MAIL;
[a-zA-Z]+	yylval.string=strdup(yytext);return STRING;
%%
