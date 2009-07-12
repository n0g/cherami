/* sasl_auth.c
 *
 * Authenticate via a saslauthd Server.
 * 
 * Author: Matthias Fassl <mf@x1598.at>
 * Date: 2009-07-09
 * License: MIT (see enclosed LICENSE file for details)
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
   
#include <net.h>

void
sasl_send_str(int socket, const char* string) {
	int hstrlen = strlen(string);
	int len = hstrlen+2;
	char buffer[len];

	char* tmp = buffer;
	unsigned short nstrlen = htons(hstrlen);
	memcpy(tmp,&nstrlen,2);
	tmp += 2;
	memcpy(tmp,string,hstrlen);
		
	if (send(socket, buffer,len, 0) == -1) {
		perror("send");
		exit(1);
	}
}

char*
sasl_receive_str(int socket) {
	int t;
	unsigned short len;
	char *string;
	
	if((t = recv(socket, &len, 2, 0)) < 1) {
		if (t < 0) perror("recv");
		else printf("Server closed connection\n");
		exit(1);
	}
	
	len = ntohs(len);
	string = malloc(len+1);
	memset(string,0,len+1);
	
	if((t = recv(socket, string, len, 0)) < 1) {
		if (t < 0) perror("recv");
		else printf("Server closed connection\n");
		exit(1);
	}
	
	return string;
}

int
sasl_auth(	const char* socket_path,
		const char* username,
		const char* password,
		const char* service,
		const char* realm) {
	int socket;
	if((socket = unix_open_socket(socket_path)) < 0)
		return 1;

	sasl_send_str(socket,username);
	sasl_send_str(socket,password);
	sasl_send_str(socket,service);
	sasl_send_str(socket,realm);
	
	char* result = sasl_receive_str(socket);
	close(socket);

	return strncmp(result,"OK",2);
}
