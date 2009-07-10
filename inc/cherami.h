int open_socket(int port, char* address, struct sockaddr* addr, socklen_t* addr_len);
int accept_connections(int socket, struct sockaddr* addr, socklen_t* addr_len);
void daemonize();
