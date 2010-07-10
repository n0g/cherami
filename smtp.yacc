%{
#include <stdio.h>
#include <string.h>
#include <config.h>

int synchronisation;
FILE *fout;
 
void yyerror(const char *str)
{
        fprintf(fout,"error: %s\n",str);
	fflush(fout);
}
 
int yywrap()
{
        return 1;
} 
  
void handle_client(int sock)
{
	FILE *fin = fdopen(sock,"r");
	fout = fdopen(sock,"w");
	/* send greeting */
	fprintf(fout,SMTP_GREETING,FQDN,VERSION);
	fflush(fout);
	yyset_in(fin);
        yyparse();
} 

%}

%token HELOTOK MAILTOK RCPTTOK DATATOK RSETTOK NOOPTOK QUITTOK MAIL STRING LT GT NL COLON
%union
{
	int number;
	char *string;
}
%token <string> MAIL
%token <string> STRING
%%
commands:
        |
	commands command
        ;

command:
	helo_command
        |
	mail_command
	|
	rcpt_command
	|
	data_command
	|
	noop_command
	|
	rset_command
	|
	quit_command
        ;

helo_command:
	HELOTOK STRING NL
	{
		fprintf(fout,"hello my client\n");
		fflush(fout);
		synchronisation = HELOTOK;
	};
mail_command:
	MAILTOK COLON LT MAIL GT NL
	{
		if(synchronisation == HELOTOK || synchronisation == DATATOK) {
			fprintf(fout,"mail command accepted: %s\n",$4);
			fflush(fout);
			synchronisation = MAILTOK;
		}
		else {
			yyerror("out of sync");
		}
	};
rcpt_command:
	RCPTTOK COLON LT MAIL GT NL
	{
		if(synchronisation == MAILTOK || synchronisation == RCPTTOK) {
			fprintf(fout,"rcpt command accepted\n");
			fflush(fout);
			synchronisation = RCPTTOK;
		}
		else {
			yyerror("out of sync");
		}
	};
data_command:
	DATATOK NL
	{
		if(synchronisation == RCPTTOK) {
			fprintf(fout,"data command received\n");
			fflush(fout);
			synchronisation = DATATOK;
		}
		else {
			yyerror("out of sync");
		}
	};
noop_command:
	NOOPTOK NL
	{
		fprintf(fout,"noop command received\n");
		fflush(fout);
	};
rset_command:
	RSETTOK NL
	{
		fprintf(fout,"delete all session data\n");
		fprintf(fout,"reset command received\n");
		fflush(fout);
	};
quit_command:
	QUITTOK NL
	{
		fprintf(fout,"quit received\n");
		fflush(fout);
	};
