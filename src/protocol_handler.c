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

void
handle_client(const int sock) {
        char hostname[HOST_NAME_MAX];
        char clienthost[HOST_NAME_MAX];
        char *from = NULL;
        char *rcpt = NULL;
        char* ip;
        char command[5];
        char* data;
	char* line;
	int authenticated = 0;

        memset(clienthost,0,HOST_NAME_MAX);
        memset(command,0,5);

        if(gethostname(hostname,HOST_NAME_MAX) < 0)
                perror("couldn't retrieve hostname");
	
	ip = getpeeraddress(sock);

        //send greeting
	tcp_send_str(sock,"220 %s ESMTP %s Ready.\r\n",hostname,VERSION);

        while((line = (char*) tcp_receive_line(sock)) != NULL) {
                //extract command
                memcpy(command,line,4);
		//convert to uppercase
		//TODO: make this nicer... own function?
                command[0] = toupper(command[0]);
                command[1] = toupper(command[1]);
                command[2] = toupper(command[2]);
                command[3] = toupper(command[3]);

		//TODO: put the actions in seperate functions
                if(strcmp(command,"EHLO") == 0 || strcmp(command,"HELO") == 0) {
			smtp_ehlo(sock,hostname,ip);
                }
                else if(strcmp(command,"AUTH") == 0) {
			authenticated = smtp_auth(sock,line);

                }
                else if(strcmp(command,"MAIL") == 0) {
			from = smtp_mail(sock,line);
                }
                else if(strcmp(command,"RCPT") == 0) {
			//TODO: what about more than 1 recipient? array of recipients?
			rcpt = smtp_rcpt(sock,from,authenticated,line);
                }
                else if(strcmp(command,"DATA") == 0) {
			int data_cnt = smtp_data(sock, rcpt, data);
			if(data < 0)
				continue;
			//TODO: add lines to the email header
                       	deliver_mail(ip,from,rcpt,data,data_cnt);
			tcp_send_str(sock, "250 OK\r\n");

			//clear from and rcpt fields, client could send another mail
			free(from);
			free(rcpt);
			from = NULL;
			rcpt = NULL;
                }
                else if(strcmp(command,"VRFY") == 0 || strcmp(command,"EXPN") == 0) {
			tcp_send_str(sock,"252 Carrier Pigeons are sworn to secrecy! *flutter*\r\n");
                }
                else if(strcmp(command,"QUIT") == 0) {
			tcp_send_str(sock, "221 %s closing connection\r\n", hostname);
                        break;
                }
                else {
			tcp_send_str(sock, "500 unrecognized command\r\n");
                }

		free(line);
        }

        shutdown(sock,2);
        exit(0);
}

char*
extract_address(char* line) {
	//TODO: check for errors - process hangs if line doesn't contain < or >
        char* address;
	if((address = strchr(line,'<')) == NULL)
		return NULL;
	address++;

	if(strchr(address, '>') == NULL)
		return NULL;	
        *(strchr(address,'>')) = '\0';

        return address;
}

int
smtp_auth_login(int socket) {
	char *recv_line;
	char *username,*password;

	//send username request (base64 encode)
	tcp_send_str(socket,"334 VXNlcm5hbWU6\r\n");
	//extract username (base64 decode it)
	recv_line = (char*) tcp_receive_line(socket);
	username = base64_decode(recv_line);
	free(recv_line);
	//send password request (base64 encoded)
	tcp_send_str(socket,"334 UGFzc3dvcmQ6\r\n");
	//extract password (base64 decode it)
	recv_line = (char*) tcp_receive_line(socket);
	password = base64_decode(recv_line);
	free(recv_line);
	//call sasl auth function
	if(!sasl_auth(SOCK_PATH,username,password,"smtp",""))
		return 1;
	else 
		return 0;
}

int
smtp_auth(int socket,char* line) {
	int authenticated = 0;
	//test for LOGIN or PLAIN type
	char* type = strchr(line,' ')+1;

	//TODO: convert type to upper case
	if(!strncmp(type,"LOGIN",5)) {
		authenticated = smtp_auth_login(socket);
	}
	else {
		tcp_send_str(socket, "504 Unrecognized authentication type\r\n");
		return authenticated;
	}

	if(authenticated)
		tcp_send_str(socket, "235 OK\r\n");
	else
		tcp_send_str(socket, "501 Authentication failed\r\n");

	return authenticated;
}

void
smtp_ehlo(int socket,char* hostname,char* ip) {
	//TODO: dynamically send available extensions
	tcp_send_str(socket, "250-%s Hello %s\r\n250 AUTH LOGIN\r\n", hostname, ip);
}

char*
smtp_mail(int socket,char* line) {
	char *from, *tmp;
	//check if mail address is well formed
	//TODO: check for at sign etc - external function check_mail_address() ?
	if((tmp = extract_address(line)) == NULL) {
		tcp_send_str(socket, "503 Mailadress Format not as expected\r\n");
		return NULL;
	}
	from = malloc(strlen(tmp)+1);
	strncpy(from,tmp,strlen(tmp));

	tcp_send_str(socket, "250 OK\r\n");

	return from;
}

char*
smtp_rcpt(int socket,char* from,int authenticated, char* line) {
	char *rcpt, *tmp;
	//check if recipient is well formed
	//TODO: check for at sign etc - external function check_mail_address() ?
	if((tmp = extract_address(line)) == NULL) {
		tcp_send_str(socket, "503 Mailadress Format not as expected\r\n");
		return NULL;
	}
	rcpt = malloc(strlen(tmp)+1);
	strncpy(rcpt,tmp,strlen(tmp));

	if(from == NULL) {
		tcp_send_str(socket, "503 I don't _have_ to talk to you. *pout*\r\n");
		return rcpt;
	}

	char* domain = strchr(rcpt,'@')+1;
	if(strncmp(domain,LOCAL_DOMAIN,strlen(LOCAL_DOMAIN)) && !authenticated) {
		tcp_send_str(socket, "550 relay not permitted\r\n");
		return NULL;
	}

	tcp_send_str(socket, "250 Accepted\r\n");
	return rcpt;
}

int 
smtp_data(int socket, char* rcpt, char* data) {
	//check if recipient is already known, otherwise smtp commands out of sequence
	if(rcpt ==  NULL) {
		tcp_send_str(socket, "503 I don't _have_ to talk to you. *pout*\r\n");
		return -1;
	}
	tcp_send_str(socket, "354 Enter message, ending with \".\" on a line by itself\r\n");

	//receive data until <CRLF>.<CRLF>
	int bytes_recv, data_size = BUF_SIZ+1, data_cnt = 0;
	char buffer[data_size], *tmp;

	data = malloc(data_size);
	tmp = data;
	//TODO: define maximum size of data
	//TODO: what about the transparency feature?
	do {
		memset(buffer,0,BUF_SIZ);
		bytes_recv = recv(socket,buffer,BUF_SIZ,0);
		data_cnt += bytes_recv;
		if(bytes_recv < 1)
			break;
		if(data_size < data_cnt) {
			data = realloc(data,data_size*2);
			data_size *= 2;
		}
		memcpy(tmp,buffer,bytes_recv);
		tmp+=bytes_recv;
	} while(strstr(data,"\n.\n") == NULL && strstr(data,"\r\n.\r\n") == NULL);

	return data_cnt;
}
