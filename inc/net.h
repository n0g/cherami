#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>

int unix_open_socket(const char* path);
int tcp_open_socket(int port, char* address, struct sockaddr_in* addr, socklen_t* addr_len);
int tcp_accept_connections(int socket);
void tcp_send_str(int socket, char* fmt, ...);
char* tcp_receive_line(int socket);
