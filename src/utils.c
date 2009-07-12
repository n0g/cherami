#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <utils.h>

/*
 * Let program fall to background. Fork 
 * process and exit parent.
 */
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

/*
 * Remove <CRLF> or <LF> from the end of a line.
 * Anything after the <CRLF> will not be readable anymore.
 */
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

/*
 * Replace all occurences of 'src' with 'dst' in char
 * sequence 'string'.
 */
char*
str_replace(const char* string, char dst, char src) {
	char *new_str = malloc(strlen(string)+1);
	strncpy(new_str,string,strlen(string));

	char *tmp;
	while((tmp = strchr(new_str, src)) != NULL)
		*tmp = dst;

	return new_str;
}

/*
 * Replace all lower case letters in char sequence
 * 'string' with their upper case version.
 */
char*
str_toupper(const char* string) {
	int str_len = strlen(string)+1;
	char *new_str = malloc(str_len);
	char *tmp = new_str;
	strncpy(new_str,string,str_len);
	
	while(*tmp != '\0')
		*tmp++ = toupper(*tmp);

	return new_str;
}

/*
 * Decode a base64 encoded STRING. This will NOT
 * work for anything other than a null-terminated
 * string.
 */
char*
base64_decode(const char* string) {
	char base64_table[66] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; 
	//replace all = by A so that base64 fill bytes end up being 
	//null terminating bytes in the decoded string
	char *new_string = str_replace(string, 'A', '=');

	char *decoded_str = malloc(1+(3*strlen(string))/4);
	char *tmp = decoded_str;

	//convert 4 base64 encoded bytes into 3 decoded bytes 
	//until the base64 encoded string ends
	int b64_1, b64_2, b64_3, b64_4;
	while(*new_string != '\0') {
		b64_1 = strchr(base64_table,(int)*new_string++)-base64_table;
		b64_2 = strchr(base64_table,(int)*new_string++)-base64_table;
		b64_3 = strchr(base64_table,(int)*new_string++)-base64_table;
		b64_4 = strchr(base64_table,(int)*new_string++)-base64_table;

		*tmp++ = (b64_1 << 2) + (b64_2 >> 4);
		*tmp++ = (b64_2 << 4) + (b64_3 >> 2);
		*tmp++ = (b64_3 << 6) + b64_4;
	
	}

	free(new_string);
	return decoded_str;
}
