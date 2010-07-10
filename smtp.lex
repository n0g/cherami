%{
#include "y.tab.h"
%}

%%
AUTH		return AUTHTOK;
HELO		return HELOTOK;
"MAIL FROM"	return MAILTOK;
"RCPT TO"	return RCPTTOK;
DATA		return DATATOK;
NOOP		return NOOPTOK;
RSET		return RSETTOK;
QUIT		return QUITTOK;
\:		return COLON;
\<		return LT;
\>		return GT;
\n		return NL;
[A-Z0-9._%-]+@[A-Z0-9.-]+\.[A-Z]{2,4}	yylval.string=strdup(yytext);return MAIL;
[A-Z]+	yylval.string=strdup(yytext);return STRING;
%%
