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
#include <glib/gbase64.h>

#include <delivery.h>
#include <protocol_handler.h>
#include <config.h>
#include <sasl_auth.h>

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
        snprintf(buffer,BUF_SIZ,"220 %s ESMTP %s Ready.\r\n",hostname,VERSION);
        send(sock,buffer,strlen(buffer),0);
        memset(buffer,0,BUF_SIZ);
        while((bytes = recv(sock,buffer,BUF_SIZ,0)) > 0)
        {
                //extract command
                memcpy(command,buffer,4);
                //convert command to upper case (for comparison
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
                        if(strlen(rcpt) == 0)
                        {
                                memset(buffer,0,BUF_SIZ);
                                snprintf(buffer,BUF_SIZ,"503 I don't _have_ to talk to you. *pout*\r\n");
                                send(sock,buffer,strlen(buffer),0);
                                memset(buffer,0,BUF_SIZ);

                                continue;
                        }
                        //354 Enter Messag, ending with "." on a line by itself 
                        memset(buffer,0,BUF_SIZ);
                        snprintf(buffer,BUF_SIZ,"354 Enter message, ending with \".\" on a line by itself\r\n");
                        send(sock,buffer,strlen(buffer),0);
                        memset(buffer,0,BUF_SIZ);

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
                        memset(buffer,0,BUF_SIZ);
                        snprintf(buffer,BUF_SIZ,"250 OK\r\n");
                        send(sock,buffer,strlen(buffer),0);
                        memset(from,0,HOST_NAME_MAX);
                        memset(rcpt,0,HOST_NAME_MAX);
                }
                else if(strcmp(command,"VRFY") == 0 || strcmp(command,"EXPN") == 0)
                {
                        memset(buffer,0,BUF_SIZ);
                        snprintf(buffer,BUF_SIZ,"252 Carrier Pigeons are sworn to secrecy! *flutter*\r\n");
                        send(sock,buffer,strlen(buffer),0);
                }
                else if(strcmp(command,"QUIT") == 0)
                {
                        memset(buffer,0,BUF_SIZ);
                        snprintf(buffer,BUF_SIZ,"221 %s closing connection\r\n",hostname);
                        send(sock,buffer,strlen(buffer),0);
                        break;
                }
                else
                {
                        memset(buffer,0,BUF_SIZ);
                        snprintf(buffer,BUF_SIZ,"500 unrecognized command\r\n");
                        send(sock,buffer,strlen(buffer),0);
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

char* getpeeraddress(int socket)
{
	char *ip = malloc(15);
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        getpeername(socket,(struct sockaddr*) &addr,&len);
        int address = addr.sin_addr.s_addr;
        int a,b,c,d;
        //TODO: this only works correctly on LE systems (and ipv4 of course)
        a = (address << 24) >> 24;
        b = (address << 16) >> 24;
        c = (address << 8) >> 24;
        d = address >> 24;
        snprintf(ip,15,"%d.%d.%d.%d",a,b,c,d);
	
	return ip;
}

char* stripCRLF(char* line)
{
	char *tmp = line;

	if((tmp = strstr(line,"\r\n")) != NULL)
		*tmp = '\0';
	else if((tmp = strchr(line,'\n')) != NULL)
		*tmp = '\0';

	return line;
}

char* base64_decode(char* string)
{
	//TODO: replace this with own implementation
	int b64_len = strlen(string);
	return g_base64_decode((gchar*)string,(gsize*)&b64_len);
}

int smtp_auth_login(int sock)
{
	char buffer[BUF_SIZ];
	char *username,*password;

	//send username request (base64 encode)
	memset(buffer,0,BUF_SIZ);
	snprintf(buffer,BUF_SIZ,"334 VXNlcm5hbWU6\r\n");
	send(sock,buffer,strlen(buffer),0);
	//extract username (base64 decode it)
	memset(buffer,0,BUF_SIZ);
	if(recv(sock,buffer,BUF_SIZ,0) < 1)
		perror("couldn't receive auth data");		
	//strip the <CRLF>/<LF> from the end
	username = base64_decode(stripCRLF(buffer));
	//send password request (base64 encoded)
	memset(buffer,0,BUF_SIZ);
	snprintf(buffer,BUF_SIZ,"334 UGFzc3dvcmQ6\r\n");
	send(sock,buffer,strlen(buffer),0);
	//extract password (base64 decode it)
	memset(buffer,0,BUF_SIZ);
	if(recv(sock,buffer,BUF_SIZ,0) < 1)
		perror("couldn't receive auth data");		
	//strip the <CRLF>/<LF> from the end
	password = base64_decode(stripCRLF(buffer));
	//call sasl auth function
	if(!authenticate(SOCK_PATH,username,password,"smtp",""))
		return 1;
	else 
		return 0;
}

int smtp_auth(int sock,char* buffer)
{
	int authenticated = 0;
	//test for LOGIN or PLAIN type
	char* type = strchr(buffer,' ')+1;

	if(!strncmp(type,"LOGIN",5))
	{
		authenticated = smtp_auth_login(sock);
	}
	else
	{
		memset(buffer,0,BUF_SIZ);
		snprintf(buffer,BUF_SIZ,"504 Unrecognized authentication type\r\n");
		send(sock,buffer,strlen(buffer),0);
		return authenticated;
	}

	memset(buffer,0,BUF_SIZ);
	if(authenticated)
	{
		snprintf(buffer,BUF_SIZ,"235 OK\r\n");
		send(sock,buffer,strlen(buffer),0);
	}
	else
	{
		snprintf(buffer,BUF_SIZ,"501 Authentication failed\r\n");
		send(sock,buffer,strlen(buffer),0);
	}

	return authenticated;
}

void smtp_ehlo(int sock,char* hostname,char* ip)
{
	char buffer[BUF_SIZ];
	memset(buffer,0,BUF_SIZ);
	snprintf(buffer,BUF_SIZ,"250-%s Hello %s\r\n250 AUTH LOGIN\r\n",hostname,ip);
	send(sock,buffer,strlen(buffer),0);
}

char* smtp_mail(int sock,char* buffer)
{
	char* tmp = extract_address(buffer);
	char* from = malloc(strlen(tmp)+1);
	strncpy(from,tmp,strlen(tmp));

	memset(buffer,0,BUF_SIZ);
	snprintf(buffer,BUF_SIZ,"250 OK\r\n");
	send(sock,buffer,strlen(buffer),0);

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
		snprintf(buffer,BUF_SIZ,"503 I don't _have_ to talk to you. *pout*\r\n");
		send(sock,buffer,strlen(buffer),0);

		return rcpt;
	}

	//TODO: check if rcpt address is well formed
	char* domain = strchr(rcpt,'@')+1;
	if(strncmp(domain,DOMAIN,strlen(DOMAIN)) && !authenticated)
	{
		snprintf(buffer,BUF_SIZ,"550 relay not permitted\r\n");
		send(sock,buffer,strlen(buffer),0);

		return NULL;
	}

	snprintf(buffer,BUF_SIZ,"250 Accepted\r\n");
	send(sock,buffer,strlen(buffer),0);

	return rcpt;
}
