/* protocol_handler.c
 *
 * Implements the server-side of the SMTP protocol.
 * 
 * Author: Matthias Fassl <mf@x1598.at>
 * Date: 2009-07-09
 * License: MIT (see enclosed LICENSE file for details)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>

#include <delivery.h>
#include <protocol_handler.h>
#include <config.h>
#include <sasl_auth.h>
#include <utils.h>

void
handle_client(const int socket) {
        char *from = NULL, *rcpt = NULL, *ip, *data;
	int authenticated = 0;

        char command[5];
	char *up_command;
	char* line;
	char *hostname = FQDN;

        memset(command,0,5);
	
	ip = getpeeraddress(socket);
	syslog(LOG_INFO, "Accepted SMTP Connection from %s",ip); 

        //send greeting
	tcp_send_str(socket,SMTP_GREETING,hostname,VERSION);

        while((line = (char*) tcp_receive_line(socket)) != NULL) {
                //extract command
                memcpy(command,line,4);
		//convert to uppercase
		up_command = str_toupper(command);

		//TODO: put the actions in seperate functions
                if(strcmp(up_command,"EHLO") == 0 || strcmp(up_command,"HELO") == 0) {
			smtp_ehlo(socket,hostname,ip);
                }
                else if(strcmp(up_command,"AUTH") == 0) {
			authenticated = smtp_auth(socket,line);
                }
                else if(strcmp(up_command,"MAIL") == 0) {
			from = smtp_mail(socket,line);
                }
                else if(strcmp(up_command,"RCPT") == 0) {
			//TODO: what about more than 1 recipient? array of recipients?
			rcpt = smtp_rcpt(socket,from,authenticated,line);
                }
                else if(strcmp(up_command,"DATA") == 0) {
			int data_cnt = smtp_data(socket, rcpt, data);
			if(data < 0)
				continue;

			//TODO: add lines to the email header
			syslog(LOG_INFO, "Accepted Email from %s to %s delivered by %s",from,rcpt,ip);
                       	deliver_mail(ip,from,rcpt,data,data_cnt);
			tcp_send_str(socket, SMTP_OK);

			//clear from and rcpt fields, client could send another mail
			free(from);
			free(rcpt);
			from = NULL;
			rcpt = NULL;
                }
                else if(strcmp(up_command,"VRFY") == 0 || strcmp(command,"EXPN") == 0) {
			syslog(LOG_INFO, "%s sent a VRFY command, he could be a spambot",ip);
			tcp_send_str(socket, SMTP_VRFY_EXPN);
                }
                else if(strcmp(up_command,"QUIT") == 0) {
			tcp_send_str(socket, SMTP_QUIT, hostname);
                        break;
                }
                else {
			syslog(LOG_INFO,"%s sent unrecognized SMTP command %s",ip,line);
			tcp_send_str(socket, SMTP_UNRECOGNIZED);
                }

		free(line);
		free(up_command);
        }

        shutdown(socket,2);
        exit(0);
}

/*
 * Extract the data betwenn first '<' and first '>'.
 * Needed in smtp_rcpt and smtp_mail to extract the
 * mail address from input lines.
 */
char*
extract_address(char* line) {
        char* address;
	if((address = strchr(line,'<')) == NULL)
		return NULL;
	address++;

	if(strchr(address, '>') == NULL)
		return NULL;	
	//TODO: don't write into that string
        *(strchr(address,'>')) = '\0';

        return address;
}

int
smtp_auth_login(const int socket) {
	char *recv_line;
	char *username,*password;

	//send username request (base64 encode)
	tcp_send_str(socket, SMTP_AUTH_LOGIN_USER);
	//extract username (base64 decode it)
	recv_line = (char*) tcp_receive_line(socket);
	username = base64_decode(recv_line);
	syslog(LOG_INFO, "%s tries to authenticate", username);
	free(recv_line);
	//send password request (base64 encoded)
	tcp_send_str(socket, SMTP_AUTH_LOGIN_PASS);
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
smtp_auth(const int socket,const char* line) {
	int authenticated = 0;
	//test for LOGIN or PLAIN type
	char* type = strchr(line,' ')+1;
	char *up_type = str_toupper(type);

	//TODO: convert type to upper case
	if(!strncmp(up_type,"LOGIN",5)) {
		authenticated = smtp_auth_login(socket);
	}
	else {
		tcp_send_str(socket, SMTP_UNRECOGNIZED_AUTH);
		free(up_type);
		return authenticated;
	}

	if(authenticated)
		tcp_send_str(socket, SMTP_AUTH_OK);
	else
		tcp_send_str(socket, SMTP_AUTH_FAIL);

	free(up_type);
	return authenticated;
}

void
smtp_ehlo(const int socket,const char* hostname,const char* ip) {
	//TODO: dynamically send available extensions
	tcp_send_str(socket, SMTP_EHLO, hostname, ip, MAX_MAIL_SIZE);
}

char*
smtp_mail(const int socket,char* line) {
	char *from, *tmp;
	//check if mail address is well formed
	//TODO: check for at sign etc - external function check_mail_address() ?
	if((tmp = extract_address(line)) == NULL) {
		tcp_send_str(socket, SMTP_UNEXPECTED_MAIL_FORMAT);
		return NULL;
	}
	from = malloc(strlen(tmp)+1);
	strncpy(from,tmp,strlen(tmp));

	tcp_send_str(socket, SMTP_OK);

	return from;
}

char*
smtp_rcpt(const int socket,const char* from,int authenticated, char* line) {
	char *rcpt, *tmp;
	if((tmp = extract_address(line)) == NULL) {
		tcp_send_str(socket, SMTP_UNEXPECTED_MAIL_FORMAT);
		return NULL;
	}
	rcpt = malloc(strlen(tmp)+1);
	strncpy(rcpt,tmp,strlen(tmp));

	if(from == NULL) {
		tcp_send_str(socket, SMTP_OUT_OF_SEQUENCE);
		return rcpt;
	}

	//TODO: well formed? check for at sign etc - external function check_mail_address() ?
	if(strchr(rcpt,'@') == NULL) {
		return NULL;
	}

	char* domain = strchr(rcpt,'@')+1;
	if(strncmp(domain,LOCAL_DOMAIN,strlen(LOCAL_DOMAIN)) && !authenticated) {
		tcp_send_str(socket, SMTP_RELAY_FORBIDDEN);
		return NULL;
	}

	tcp_send_str(socket, SMTP_ACCEPTED);
	return rcpt;
}

int 
smtp_data(const int socket, const char* rcpt, char* data) {
	//check if recipient is already known, otherwise smtp commands out of sequence
	if(rcpt ==  NULL) {
		tcp_send_str(socket, SMTP_OUT_OF_SEQUENCE);
		return -1;
	}
	tcp_send_str(socket, SMTP_ENTER_MESSAGE);

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
