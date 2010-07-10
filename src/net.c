#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <syslog.h>

#include <config.h>
#include <net.h>
#include <utils.h>

#define LISTEN_QUEUE 128

int
unix_open_socket(const char* path) {
        int s, len;
        struct sockaddr_un remote;

        if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		syslog(LOG_ERR,"unix_open_socket: couldn't create unix socket");
		return -1;
        }

        remote.sun_family = AF_UNIX;
        strncpy(remote.sun_path, path, sizeof(remote.sun_path));
        len = strlen(remote.sun_path) + sizeof(remote.sun_family);
        if (connect(s, (struct sockaddr *)&remote, len) == -1) {
		syslog(LOG_ERR, "unix_open_socket: couldn't connect to %s",path);
		return -2;
        }

        return s;
}

tcp_open_server_socket(const char *hostname,
              const char *service,
              int         family,
              int         socktype)
{
    struct addrinfo hints, *res, *ressave;
    int n, sockfd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags    = AI_PASSIVE;
    hints.ai_family   = family;
    hints.ai_socktype = socktype;

    if((n = getaddrinfo(hostname, service, &hints, &res)) != 0) {
	syslog(LOG_ERR,"tcp_open_server_socket: getaddrinfo error:: [%s]", gai_strerror(n));
        return -1;
    }

    ressave=res;

    /*
       Try open socket with each address getaddrinfo returned,
       until getting a valid listening socket.
    */
    sockfd=-1;
    while (res) {
        if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) != -1) {
            if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
                break;

            close(sockfd);
            sockfd=-1;
        }
        res = res->ai_next;
    }

	/* tried all sockets and it didn't work */
    if (sockfd < 0) {
        freeaddrinfo(ressave);
	syslog(LOG_ERR, "tcp_open_server_socket: Could not open socket");
        return -1;
    }

    listen(sockfd, LISTEN_QUEUE);

    freeaddrinfo(ressave);
    return sockfd;
}

int
tcp_accept_connections(int socket, void (*fp)(const int socket)) {
	struct sockaddr_storage addr;
	socklen_t addr_len = sizeof(addr);

	int pid, c;
	for(;;) {
		if((c = accept(socket, (struct sockaddr*) &addr, &addr_len)) == -1) {
			syslog(LOG_ERR, "tcp_accept_connections: accept() failed, probably because socket was closed - terminating");
			break;
		}

		if((pid = fork()) == 0) {
			(*fp)(c);
		}
		else if(pid < 0) {
			syslog(LOG_ERR, "tcp_accept_connections: fork() failed. You're probably fucked.");
		}
	}

	return 0;
}
