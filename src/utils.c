#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <utils.h>

void
daemonize() {
        int pid;
        pid = fork();
        if (pid > 0)
        {
                //syslog(LOG_INFO,"Exiting Parent");
                exit(0);
        }
}

char*
stripCRLF(char* line) {
        char *tmp = line;

        if((tmp = strstr(line,"\r\n")) != NULL)
                *tmp = '\0';
        else if((tmp = strchr(line,'\n')) != NULL)
                *tmp = '\0';

        return line;
}

char*
getpeeraddress(int socket) {
        char *ip = malloc(15);
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        getpeername(socket,(struct sockaddr*) &addr,&len);
        int address = addr.sin_addr.s_addr;
        int a,b,c,d;
        //TODO: this only works correctly on LE systems (and ipv4 of course)
        a = (address << 24) >> 24;
        b = (address << 16) >> 24;
        c = (address << 8) >> 24;
        d = address >> 24;
        snprintf(ip,15,"%d.%d.%d.%d",a,b,c,d);

        return ip;
}

char*
base64_decode(char* string) {
	char *new_string = malloc(strlen(string)+1);
	strncpy(new_string,string,strlen(string));

	char base64_table[66] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; 

	char *decoded_str = malloc(1+(3*strlen(string))/4);
	char *tmp = decoded_str;
	
	//replace all = by A so that base64 fill bytes end up being 
	//null terminating bytes in the decoded string
	char *blah;
	while((blah = strchr(new_string,'=')) != NULL) {
		*blah = 'A'; 
	}

	//convert 4 base64 encoded bytes into 3 decoded bytes 
	//until the base64 encoded string ends
	while(*new_string != '\0') {
		int v1 = strchr(base64_table,(int)*new_string++)-base64_table;
		int v2 = strchr(base64_table,(int)*new_string++)-base64_table;
		int v3 = strchr(base64_table,(int)*new_string++)-base64_table;
		int v4 = strchr(base64_table,(int)*new_string++)-base64_table;

		*tmp++ = (v1 << 2) + (v2 >> 4);
		*tmp++ = (v2 << 4) + (v3 >> 2);
		*tmp++ = (v3 << 6) + v4;
	
	}

	free(new_string);
	return decoded_str;
}
