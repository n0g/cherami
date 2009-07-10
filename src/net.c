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
tcp_open_socket(int port, char* address, struct sockaddr* addr, socklen_t* addr_len) {
    int s, c;

    //create socket
    if((s = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket() failed");
        return -1;
    }

    //TODO: use address string to specify where to open the socket
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr_len = sizeof(addr);

    //bind socket
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
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
tcp_accept_connections(int socket, struct sockaddr* addr, socklen_t* addr) {
    int pid;
    for(;;) {
        if((c = accept(socket, addr, addr_len)) == -1) {
                perror("accept() failed, probably because socket was closed - terminating");
                break;
        }

        if((pid = fork()) == 0) {
                handle_client(c);
        }
        else if(pid < 0) {
                perror("fork() failed");
        }
   }

   return 0;
}

