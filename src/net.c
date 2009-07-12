#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>

#include <config.h>
#include <net.h>
#include <utils.h>
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
        strncpy(remote.sun_path, path, sizeof(remote.sun_path));
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
	va_list ap, ap_cpy;
	int d;
	char c, *s, *buffer, *fmt_cpy;

	int len = strlen(fmt)+1;

	va_start(ap, fmt);

	fmt_cpy = fmt;
	va_copy(ap_cpy, ap);

	//check how much space we need to allocate
	while (*fmt)
		switch (*fmt++) {
			case 's':
				s = va_arg(ap, char *);
				if(*(fmt-2) == '%') len += strlen(s);
				break;
			case 'd':
				d = va_arg(ap, int);
				if(*(fmt-2) == '%') len += (int) (floor(log10((double) d))+1.0);
				break;
		}

	//allocate the correct amount of space (actually a bit more)
	buffer = malloc(len);
	memset(buffer,0,len);

	//print everything to the buffer
	vsnprintf(buffer, len, fmt_cpy, ap_cpy);
	//send buffer
	send(socket, buffer, strlen(buffer), 0);

	free(buffer);
	va_end(ap_cpy);
	va_end(ap);
}

char*
tcp_receive_line(int socket) {
	int bytes_recv, data_size = BUF_SIZ+1, data_cnt = 0;
	char buffer[BUF_SIZ], *data = malloc(data_size), *tmp;

	tmp = data;
	//receive data until a newline occurs
	//TODO: yeeeeeaaah, this really does *not* work -> repair
	do {
		memset(buffer, 0, BUF_SIZ);
		bytes_recv = recv(socket, buffer, BUF_SIZ, 0);
		data_cnt += bytes_recv;
		//if number of received bytes is 0 or less, an error occured 
		//and we want to stop reading form the socket
		if(bytes_recv < 1) {
			return NULL;
		}
		//if the number of bytes we received is bigger than our allocated 
		//space we need to enlarge the reserved memory for it
		if(data_size < data_cnt)
			data = realloc(data, data_size *= 2);

		memcpy(tmp, buffer, bytes_recv);
		tmp += bytes_recv;
	} while(strstr(data, "\n") == NULL);

	return stripCRLF(data);	
}
