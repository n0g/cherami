/* sasl_auth.c
Copyright (c) 2009 Matthias Fassl <mf@x1598.at>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
   
int
open_socket(const char* path) {	
	int s, len;
	struct sockaddr_un remote;

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, path);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (connect(s, (struct sockaddr *)&remote, len) == -1) {
		perror("connect");
		exit(1);
	}
	
	return s;
}

void
send_string(int socket, const char* string) {
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
receive_string(int socket) {
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
authenticate(	const char* socket_path,
		const char* username,
		const char* password,
		const char* service,
		const char* realm) {
	int socket = open_socket(socket_path);

	send_string(socket,username);
	send_string(socket,password);
	send_string(socket,service);
	send_string(socket,realm);
	
	char* result = receive_string(socket);
	close(socket);

	return strncmp(result,"OK",2);
}
