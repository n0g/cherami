#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>

#include <net.h>
#include <protocol_handler.h>

int
unix_open_socket(const char* path) {
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

int
tcp_open_socket(int port, char* address, struct sockaddr_in* addr, socklen_t* addr_len) {
	int s;

	//TODO: use address string to specify where to open the socket
	(*addr).sin_addr.s_addr = INADDR_ANY;
	(*addr).sin_port = htons(port);
	(*addr).sin_family = AF_INET;
	*addr_len = sizeof(*addr);

	//create socket
	if((s = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket() failed");
		return -1;
	}


	//bind socket
	if (bind(s, (struct sockaddr*)addr, *addr_len) == -1) {
		perror("bind() failed");
		return -2;
	}

	//listen socket
	if (listen(s, 3) == -1) {
		perror("listen() failed");
		return -3;
	}

	return s;
}

int
tcp_accept_connections(int socket, struct sockaddr_in* addr, socklen_t* addr_len) {
	int pid, c;
	for(;;) {
		if((c = accept(socket, (struct sockaddr*) addr, addr_len)) == -1) {
			perror("accept() failed, probably because socket was closed - terminating");
			break;
		}

		if((pid = fork()) == 0) {
			//TODO: function pointer for handle client
			handle_client(c);
		}
		else if(pid < 0) {
			perror("fork() failed");
		}
	}

	return 0;
}

void
tcp_send_str(int socket, char* fmt, ...) {
	va_list ap, copy;
	int d;
	char c, *s, *buffer, *tmp;

	int len = strlen(fmt)+1;

	va_start(ap, fmt);
	tmp = fmt;
	va_copy(copy, ap);
	while (*fmt)
		switch (*fmt++) {
			case 's':
				s = va_arg(ap, char *);
				len += strlen(s);
				break;
			case 'd':
				d = va_arg(ap, int);
				len += (int) (floor(log10((double) d))+1.0);
				break;
			case 'c':
				c = (char) va_arg(ap, int);
				len ++;
				break;
		}
	fmt = tmp;

	buffer = malloc(len);
	memset(buffer,0,len);
	vsnprintf(buffer, len, fmt, copy);
	send(socket, buffer, strlen(buffer), 0);
	free(buffer);
	va_end(copy);
	va_end(ap);
}
