#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sasl_auth.h>
#include <config.h>

void handle_client(const int sock);
char* extract_address(char* line);
int deliver_mail(char* ip,char* from,char* rcpt,char* data,int datalen);

int main(void)
{
    int s, c;
    socklen_t addr_len;
    struct sockaddr_in addr;

    //create socket
    if((s = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() failed");
        return 1;
    }

    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    addr.sin_family = AF_INET;
    addr_len = sizeof(addr);

    //bind socket
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind() failed");
        return 2;
    }

    //listen socket
    if (listen(s, 3) == -1)
    {
        perror("listen() failed");
        return 3;
    }

    int pid;
    //accept client connections
    for(;;)
    {
        if((c = accept(s, (struct sockaddr*)&addr, &addr_len)) == -1)
        {
            	perror("accept() failed");
            	continue;
        }
	
	if((pid = fork()) == 0)
        {
		handle_client(c);
        }
	else if(pid < 0)
        {
		perror("fork() failed");
        }
	

    }

    close(s);
    return 0;
}

void handle_client(const int sock)
{
	char buffer[BUF_SIZ];
	char hostname[HOST_NAME_MAX];
	char clienthost[HOST_NAME_MAX];
	char from[HOST_NAME_MAX];
	char rcpt[HOST_NAME_MAX];
	char ip[15];;
	char command[5];
	char* data;
	int bytes;

	memset(buffer,0,BUF_SIZ);
	memset(clienthost,0,HOST_NAME_MAX);
	memset(from,0,HOST_NAME_MAX);
	memset(rcpt,0,HOST_NAME_MAX);
	memset(command,0,5);

	if(gethostname(hostname,HOST_NAME_MAX) < 0)
        {
		perror("couldn't retrieve hostname");
        }

	//get ip address of peer
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getpeername(sock,(struct sockaddr*) &addr,&len);
	int address = addr.sin_addr.s_addr;
	int a,b,c,d;
	//this only works correctly on LE systems (and ipv4 of course)
	a = (address << 24) >> 24;
	b = (address << 16) >> 24;
	c = (address << 8) >> 24; 
	d = address >> 24;		
	snprintf(ip,15,"%d.%d.%d.%d",a,b,c,d);
	
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
		
		if(strcmp(command,"EHLO") == 0 || strcmp(command,"HELO") == 0)
       		{
			memset(buffer,0,BUF_SIZ);
			snprintf(buffer,BUF_SIZ,"250 %s Hello to you, Sir! %s. It's a great day to save lives. \r\n",hostname,ip);	
			//send supported extensions	
			
			send(sock,buffer,strlen(buffer),0);
			memset(buffer,0,BUF_SIZ);
			continue;
        	}
		else if(strcmp(command,"AUTH") == 0)
		{
			continue;	
		}
		else if(strcmp(command,"MAIL") == 0)
        	{
			char* tmp = extract_address(buffer); 
			memcpy(from,tmp,strlen(tmp));

			memset(buffer,0,BUF_SIZ);
			snprintf(buffer,BUF_SIZ,"250 Understood.\r\n");
			send(sock,buffer,strlen(buffer),0);
			memset(buffer,0,BUF_SIZ);
			continue;
        	}
		else if(strcmp(command,"RCPT") == 0)
        	{
			if(strlen(from) == 0)
			{
				memset(buffer,0,BUF_SIZ);
				snprintf(buffer,BUF_SIZ,"503 I don't _have_ to talk to you. *pout*\r\n");
				send(sock,buffer,strlen(buffer),0);
				memset(buffer,0,BUF_SIZ);
				
				continue;
			}
			char* tmp = extract_address(buffer); 
			memcpy(rcpt,tmp,strlen(tmp));

			
			memset(buffer,0,BUF_SIZ);
			snprintf(buffer,BUF_SIZ,"250 That's pretty far. I don't know if i can make it 'till there. I'll give my best!\r\n");
			send(sock,buffer,strlen(buffer),0);
			memset(buffer,0,BUF_SIZ);
			continue;
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
			snprintf(buffer,BUF_SIZ,"354 Write a message on a piece of paper and connect it to my foot by ending it with \".\" on a line by itself\r\n");
			send(sock,buffer,strlen(buffer),0);
			memset(buffer,0,BUF_SIZ);

			//receive data until <CRLF>.<CRLF>
			data = malloc(BUF_SIZ+1);
			char* tmp = data;
			int datacounter = 0;
			int sizeofdata = BUF_SIZ+1;
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

			deliver_mail(ip,from,rcpt,data,datacounter);
			//250 OK
			memset(buffer,0,BUF_SIZ);
			snprintf(buffer,BUF_SIZ,"250 OK got it. Giving my best to save the \"Lost Battalion\".\r\n");
			send(sock,buffer,strlen(buffer),0);
			memset(buffer,0,BUF_SIZ);
			memset(from,0,HOST_NAME_MAX);
			memset(rcpt,0,HOST_NAME_MAX);
			continue;
        	}
		else if(strcmp(command,"VRFY") == 0 || strcmp(command,"EXPN") == 0)
		{
			memset(buffer,0,BUF_SIZ);
			snprintf(buffer,BUF_SIZ,"252 Carrier Pigeons are sworn to secrecy! *flutter*\r\n");
			send(sock,buffer,strlen(buffer),0);
			memset(buffer,0,BUF_SIZ);
			continue;
		}
		else if(strcmp(command,"QUIT") == 0)
        	{
			memset(buffer,0,BUF_SIZ);
			snprintf(buffer,BUF_SIZ,"221 %s Have a nice day, Sir!\r\n",hostname);
			send(sock,buffer,strlen(buffer),0);
			memset(buffer,0,BUF_SIZ);
			break;
        	}
		else
		{
			memset(buffer,0,BUF_SIZ);
			snprintf(buffer,BUF_SIZ,"500 I have _no_ idea what you are talking about.\r\n");
			send(sock,buffer,strlen(buffer),0);
			memset(buffer,0,BUF_SIZ);
			continue;
		}
        }

	shutdown(sock,2);
	exit(0);
}

char* extract_address(char* line)
{
	char* address = strchr(line,'<')+1;
	*(strchr(line,'>')) = '\0';
	
	return address;
}

int deliver_mail(char* ip,char* from,char* rcpt,char* data,int datalen)
{
	char* domain = strchr(rcpt,'@')+1;
	printf("IP-Address: %s\nFrom: %s\nRecipient: %s\nDomain: %s\nData:\n%s\n",ip,from,rcpt,domain,data);
	
	if(strcmp(domain,DOMAIN) == 0)
	{
		//local delivery
		return deliver_local(ip,from,rcpt,data,datalen);
	}
	else
	{
		//relay
		return deliver_relay(ip,domain,from,rcpt,data,datalen);
	}
}

int deliver_local(char* ip,char* from,char* rcpt,char* data,int datalen)
{
	printf("local delivery\n");
	*(strchr(rcpt,'@')) = '\0';
	printf("user: %s\n",rcpt);
	return 0;
}

int deliver_relay(char* ip,char* targetdomain,char* from,char* rcpt,char* data,int datalen)
{
	printf("relay delivery\n");
	//get mailserver for targetdomain
	//open socket to mailserver
	//protocolhandling
	return 0;
}
