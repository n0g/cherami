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
#include <syslog.h>
#include <signal.h>

#include <config.h>
#include <net.h>
#include <utils.h>

int
main(int argc, char* argv[]) {
	//TODO: read commandline options somewhere

	openlog(argv[0], LOG_PID|LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO,"----- %s is starting -----",argv[0]);

	int socket = 0;
	if((socket = tcp_open_server_socket(LISTEN_ADDRESS,PORT,AF_UNSPEC,SOCK_STREAM)) < 0) {
		syslog(LOG_ERR,"Couldn't open Socket on %s %s",LISTEN_ADDRESS,PORT);
		exit(1);
	}

	daemonize();
	//TODO: write PID file
	tcp_accept_connections(socket);
	//TODO: install signal handler to close socket
	//signal( SIGTERM, sig_handler);
	//close(s);
	//TODO: closelog for syslog
	return 0;
}
