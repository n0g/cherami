int unix_open_socket(const char* path);
int tcp_open_socket(int port, char* address, struct sockaddr* addr, socklen_t* addr_len);
int tcp_accept_connections(int socket, struct sockaddr* addr, socklen_t* addr_len);
