/* cherami.c
 *
 * Interpret command line options, open the socket 
 * and daemonize mailserver.
 * 
 * Author: Matthias Fassl <mf@x1598.at>
 * Date: 2009-07-09
 * License: MIT (see enclosed LICENSE file for details)
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include <protocol_handler.h>
#include <config.h>
#include <cherami.h>

int main(int argc, char* argv)
{
   //TODO: read commandline options somewhere
   //TODO: add syslog logging here
   //openlog(argv[0], LOG_PID|LOG_CONS, LOG_DAEMON);
   //syslog(LOG_INFO,"----- %s is starting -----",argv[0]);
   socklen_t addr_len;
   struct sockaddr_in addr;

   int socket = 0;
   if((socket = open_socket(PORT,"0.0.0.0",&addr,&addr_len)) < 0)
   {
	perror("couldn't open socket");
	exit(1);
   }    

    daemonize();
    accept_connectios(socket,&addr,&addr_len);
    //TODO: install signal handler to close socket
    //signal( SIGTERM, sig_handler);
    close(s);
    return 0;
}

int open_socket(int port, char* address, struct sockaddr* addr, socklen_t* addr_len)
{
    int s, c;

    //create socket
    if((s = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() failed");
        return -1;
    }

    //TODO: use address string to specify where to open the socket
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr_len = sizeof(addr);

    //bind socket
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind() failed");
        return -2;
    }

    //listen socket
    if (listen(s, 3) == -1)
    {
        perror("listen() failed");
        return -3;
    }

   return s;
}

int accept_connections(int socket, struct sockaddr* addr, socklen_t* addr)
{
    int pid;
    //accept client connections
    for(;;)
    {
        if((c = accept(socket, addr, addr_len)) == -1)
        {
            	perror("accept() failed, probably because socket was closed - terminating");
		break;
        }
	
	if((pid = fork()) == 0)
        {
		handle_client(c);
        }
	else if(pid < 0)
        {
		perror("fork() failed");
        }
   }

   return 0;
}

void daemonize()
{
        int pid;
        pid = fork();
        if (pid > 0) 
	{
                //syslog(LOG_INFO,"Exiting Parent");
                exit(0);
        }
}
