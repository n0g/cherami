#include <sys/socket.h>
#include <arpa/inet.h>

int unix_open_socket(const char* path);
int tcp_open_socket(int port, char* address, struct sockaddr_in* addr, socklen_t* addr_len);
int tcp_accept_connections(int socket, struct sockaddr_in* addr, socklen_t* addr_len);
