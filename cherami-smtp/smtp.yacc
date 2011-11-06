%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <config.h>
#include <utils.h>
#include <sasl_auth.h>

int synchronisation, s, authenticated;
FILE *fout, *fin, *data;
char *ip, *from, *rcpt;
 
void yyerror(const char *str)
{
        fprintf(fout,"%s",str);
	fflush(fout);
}
 
int yywrap()
{
        return 1;
} 
  
void handle_client(int sock)
{
	fin = fdopen(sock,"r");
	fout = fdopen(sock,"w");
	ip = getpeeraddress(sock);
	s = sock;
	authenticated = 0;
	from = rcpt = NULL;

	fprintf(fout,SMTP_GREETING,FQDN,VERSION);
	fflush(fout);

	yyset_in(fin);
        yyparse();
} 

%}

%token AUTHTOK HELOTOK MAILTOK RCPTTOK DATATOK RSETTOK NOOPTOK QUITTOK MAIL STRING LT GT NL COLON
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
	auth_command
        |
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
		fprintf(fout,SMTP_EHLO,FQDN,ip,MAX_MAIL_SIZE);
		fflush(fout);
		synchronisation = HELOTOK;
	};
auth_command:
	AUTHTOK STRING NL
	{
		char *username, *password;
		char line[1024];

		if(strcasecmp($2,"LOGIN") != 0 ) {
			fprintf(fout,SMTP_UNRECOGNIZED_AUTH);
			fflush(fout);
			break;
		}

		fprintf(fout,SMTP_AUTH_LOGIN_USER);
		fflush(fout);
		fgets(line,1024,fin);
		username = base64_decode(line);

		fprintf(fout,SMTP_AUTH_LOGIN_PASS);
		fflush(fout);
		fgets(line,1024,fin);
		password = base64_decode(line);

		/*
		if(!sasl_auth(SOCK_PATH,username,password,"smtp",""))
			authenticated = 1;
		
		*/
		
		free(username);
		free(password);
	};
mail_command:
	MAILTOK COLON LT MAIL GT NL
	{
		if(synchronisation == HELOTOK || synchronisation == DATATOK) {
			from = strdup($4);
			fprintf(fout,SMTP_OK);
			fflush(fout);
			synchronisation = MAILTOK;
		}
		else {
			yyerror(SMTP_OUT_OF_SEQUENCE);
		}
	};
rcpt_command:
	RCPTTOK COLON LT MAIL GT NL
	{
		/* allow more than one recipient */
		if(synchronisation == MAILTOK) {
			char *domain = strrchr($4,'@');
			domain++;
			if(strcasecmp(domain,LOCAL_DOMAIN) == 0 || (strcasecmp(domain,LOCAL_DOMAIN) != 0 && authenticated)) {
				rcpt = strdup($4);
				fprintf(fout,SMTP_ACCEPTED);
			}
			else {
				fprintf(fout,SMTP_RELAY_FORBIDDEN);
			}
			fflush(fout);
			synchronisation = RCPTTOK;
		}
		else {
			yyerror(SMTP_OUT_OF_SEQUENCE);
		}
	};
data_command:
	DATATOK NL
	{
		char buf[1024];
		int len;
		if(synchronisation == RCPTTOK) {
			fprintf(fout,SMTP_ENTER_MESSAGE);
			fflush(fout);
			/* open file in spool directory */
			/* TODO: useful file name */
			/* TODO: enforce max file limit */
			data = fopen("test","w");
			/* read data */
			do {
				fgets(buf,1024,fin);
				/* transparency */
				if(strncmp(buf,"..",2) == 0) {
					fputs(buf+1,data);
				}
				else {
					fputs(buf,data);	
				}
			} while(strncmp(buf,".\r\n",3) !=0 && strncmp(buf,".\n",2) != 0);

			fprintf(fout,SMTP_ACCEPTED);
			fprintf(fout,"Delivering Mail from %s (connected by %s) to %s\n",from,ip,rcpt);
			fflush(fout);
			synchronisation = DATATOK;
		}
		else {
			yyerror(SMTP_OUT_OF_SEQUENCE);
		}
	};
noop_command:
	NOOPTOK NL
	{
		fprintf(fout,SMTP_OK);
		fflush(fout);
	};
rset_command:
	RSETTOK NL
	{
		if(from != NULL) {
			free(from);
			from = NULL;
		}
		if(rcpt != NULL) {
			free(rcpt);
			rcpt = NULL;
		}
		fprintf(fout,SMTP_OK);
		fflush(fout);
	};
quit_command:
	QUITTOK NL
	{
		fprintf(fout,SMTP_QUIT,FQDN);
		fflush(fout);
		shutdown(s, SHUT_RDWR);
		exit(EXIT_SUCCESS);
	};
