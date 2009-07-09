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

#include <protocol_handler.h>
#include <config.h>

int main(int argc, char* argv)
{
    int s, c;
    socklen_t addr_len;
    struct sockaddr_in addr;

    //create socket
    if((s = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() failed");
        return 1;
    }

    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    addr.sin_family = AF_INET;
    addr_len = sizeof(addr);

    //bind socket
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind() failed");
        return 2;
    }

    //listen socket
    if (listen(s, 3) == -1)
    {
        perror("listen() failed");
        return 3;
    }

    int pid;
    //accept client connections
    for(;;)
    {
        if((c = accept(s, (struct sockaddr*)&addr, &addr_len)) == -1)
        {
            	perror("accept() failed");
            	continue;
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

    close(s);
    return 0;
}
