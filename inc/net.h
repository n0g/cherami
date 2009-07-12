#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>

int unix_open_socket(const char* path);
int tcp_accept_connections(int socket);
tcp_open_server_socket(const char *hostname, const char *service, int family, int socktype);
void tcp_send_str(int socket, char* fmt, ...);
char* tcp_receive_line(int socket);
