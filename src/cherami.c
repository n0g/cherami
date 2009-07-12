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
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>

#include <config.h>
#include <net.h>
#include <utils.h>

int
main(int argc, char* argv) {
   //TODO: read commandline options somewhere
   //TODO: add syslog logging here
   //openlog(argv[0], LOG_PID|LOG_CONS, LOG_DAEMON);
   //syslog(LOG_INFO,"----- %s is starting -----",argv[0]);
   socklen_t addr_len;
   struct sockaddr_in addr;

   int socket = 0;
   if((socket = tcp_open_socket(PORT,"0.0.0.0",&addr,&addr_len)) < 0) {
	perror("couldn't open socket");
	exit(1);
   }    

    daemonize();
    //TODO: write PID file
    tcp_accept_connections(socket,&addr,&addr_len);
    //TODO: install signal handler to close socket
    //signal( SIGTERM, sig_handler);
    //close(s);
    return 0;
}
