/* protocol_handler.c
 *
 * Implements the server-side of the SMTP protocol.
 * 
 * Author: Matthias Fassl <mf@x1598.at>
 * Date: 2009-07-09
 * License: MIT (see enclosed LICENSE file for details)
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <delivery.h>
#include <protocol_handler.h>
#include <config.h>
#include <sasl_auth.h>
#include <utils.h>

void handle_client(const int sock)
{
        char buffer[BUF_SIZ];
        char hostname[HOST_NAME_MAX];
        char clienthost[HOST_NAME_MAX];
        char *from = NULL;
        char *rcpt = NULL;
        char* ip;
        char command[5];
        char* data;
        int bytes;
	int authenticated = 0;

        memset(buffer,0,BUF_SIZ);
        memset(clienthost,0,HOST_NAME_MAX);
        memset(command,0,5);

        if(gethostname(hostname,HOST_NAME_MAX) < 0)
        {
                perror("couldn't retrieve hostname");
        }
	
	ip = getpeeraddress(sock);

        //send greeting
	tcp_send_str(sock,"220 %s ESMTP %s Ready.\r\n",hostname,VERSION);

        while((bytes = recv(sock,buffer,BUF_SIZ,0)) > 0)
        {
                //extract command
                memcpy(command,buffer,4);
		//convert to uppercase
		//TODO: make this nicer... own function?
                command[0] = toupper(command[0]);
                command[1] = toupper(command[1]);
                command[2] = toupper(command[2]);
                command[3] = toupper(command[3]);

		//TODO: put the actions in seperate functions
                if(strcmp(command,"EHLO") == 0 || strcmp(command,"HELO") == 0)
                {
			smtp_ehlo(sock,hostname,ip);
                }
                else if(strcmp(command,"AUTH") == 0)
                {
			authenticated = smtp_auth(sock,buffer);

                }
                else if(strcmp(command,"MAIL") == 0)
                {
			from = smtp_mail(sock,buffer);
                }
                else if(strcmp(command,"RCPT") == 0)
                {
			//TODO: what about more than 1 recipient? array of recipients?
			rcpt = smtp_rcpt(sock,from,authenticated,buffer);
                }
                else if(strcmp(command,"DATA") == 0)
                {
			//check if recipient is already known, otherwise smtp commands out of sequence
                        if(rcpt ==  NULL)
                        {
				tcp_send_str(sock, "503 I don't _have_ to talk to you. *pout*\r\n");

				memset(buffer,0,BUF_SIZ);
                                continue;
                        }
			tcp_send_str(sock, "354 Enter message, ending with \".\" on a line by itself\r\n");

                        //receive data until <CRLF>.<CRLF>
                        data = malloc(BUF_SIZ+1);
                        char* tmp = data;
                        int datacounter = 0;
                        int sizeofdata = BUF_SIZ+1;
			//TODO: define maximum size of data
			//TODO: what about the transparency feature?
                        do {
                                memset(buffer,0,BUF_SIZ);
                                bytes = recv(sock,buffer,BUF_SIZ,0);
                                datacounter+=bytes;
                                if(bytes < 1)
                                        break;
                                if(sizeofdata < datacounter)
                                {
                                        data = realloc(data,sizeofdata*2);
                                        sizeofdata *= 2;
                                }
                                memcpy(tmp,buffer,bytes);
                                tmp+=bytes;

                        } while(strstr(data,"\n.\n") == NULL && strstr(data,"\r\n.\r\n") == NULL);

			//TODO: add lines to the email header
                        deliver_mail(ip,from,rcpt,data,datacounter);
                        //250 OK
			tcp_send_str(sock, "250 OK\r\n");

			from = NULL;
			rcpt = NULL;
                }
                else if(strcmp(command,"VRFY") == 0 || strcmp(command,"EXPN") == 0)
                {
			tcp_send_str(sock,"252 Carrier Pigeons are sworn to secrecy! *flutter*\r\n");
                }
                else if(strcmp(command,"QUIT") == 0)
                {
			tcp_send_str(sock, "221 %s closing connection\r\n", hostname);
                        break;
                }
                else
                {
			tcp_send_str(sock, "500 unrecognized command\r\n");
                }

		//clear buffer for next round
                memset(buffer,0,BUF_SIZ);
        }

        shutdown(sock,2);
        exit(0);
}

char* extract_address(char* line)
{
	//TODO: check for errors - process hangs if line doesn't contain < or >
        char* address = strchr(line,'<')+1;
        *(strchr(line,'>')) = '\0';

        return address;
}

int smtp_auth_login(int sock)
{
	char buffer[BUF_SIZ];
	char *username,*password;

	//send username request (base64 encode)
	tcp_send_str(sock,"334 VXNlcm5hbWU6\r\n");
	//extract username (base64 decode it)
	memset(buffer,0,BUF_SIZ);
	if(recv(sock,buffer,BUF_SIZ,0) < 1)
		perror("couldn't receive auth data");		
	//strip the <CRLF>/<LF> from the end
	username = base64_decode(stripCRLF(buffer));
	//send password request (base64 encoded)
	tcp_send_str(sock,"334 UGFzc3dvcmQ6\r\n");
	//extract password (base64 decode it)
	memset(buffer,0,BUF_SIZ);
	if(recv(sock,buffer,BUF_SIZ,0) < 1)
		perror("couldn't receive auth data");		
	//strip the <CRLF>/<LF> from the end
	password = base64_decode(stripCRLF(buffer));
	//call sasl auth function
	if(!sasl_auth(SOCK_PATH,username,password,"smtp",""))
		return 1;
	else 
		return 0;
}

int smtp_auth(int sock,char* line)
{
	int authenticated = 0;
	//test for LOGIN or PLAIN type
	char* type = strchr(line,' ')+1;

	//TODO: convert type to upper case
	if(!strncmp(type,"LOGIN",5))
	{
		authenticated = smtp_auth_login(sock);
	}
	else
	{
		tcp_send_str(sock, "504 Unrecognized authentication type\r\n");
		return authenticated;
	}

	if(authenticated)
		tcp_send_str(sock, "235 OK\r\n");
	else
		tcp_send_str(sock, "501 Authentication failed\r\n");

	return authenticated;
}

void smtp_ehlo(int sock,char* hostname,char* ip)
{
	//TODO: dynamically send available extensions
	tcp_send_str(sock, "250-%s Hello %s\r\n250 AUTH LOGIN\r\n", hostname, ip);
}

char* smtp_mail(int sock,char* buffer)
{
	char* tmp = extract_address(buffer);
	char* from = malloc(strlen(tmp)+1);
	strncpy(from,tmp,strlen(tmp));

	tcp_send_str(sock, "250 OK\r\n");

	return from;
}

char* smtp_rcpt(int sock,char* from,int authenticated, char* buffer)
{
	char* rcpt = NULL;
	char* tmp = extract_address(buffer);
	rcpt = malloc(strlen(tmp)+1);
	strncpy(rcpt,tmp,strlen(tmp));

	memset(buffer,0,BUF_SIZ);
	if(from = NULL)
	{
		tcp_send_str(sock, "503 I don't _have_ to talk to you. *pout*\r\n");
		return rcpt;
	}

	//TODO: check if rcpt address is well formed
	char* domain = strchr(rcpt,'@')+1;
	if(strncmp(domain,LOCAL_DOMAIN,strlen(LOCAL_DOMAIN)) && !authenticated)
	{
		tcp_send_str(sock, "550 relay not permitted\r\n");
		return NULL;
	}

	tcp_send_str(sock, "250 Accepted\r\n");
	return rcpt;
}
