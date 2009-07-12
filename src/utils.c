#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>

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
                syslog(LOG_INFO,"daemonize: Exiting Parent");
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
	struct sockaddr_storage ss;
	socklen_t sslen = sizeof(ss);
	char *numeric_name = malloc(NI_MAXHOST);
	char *name = malloc(NI_MAXHOST);

	getpeername(socket, (struct sockaddr *)&ss, &sslen);
	getnameinfo((struct sockaddr *)&ss, sslen, numeric_name, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
	getnameinfo((struct sockaddr *)&ss, sslen, name, NI_MAXHOST, NULL, 0, NI_NAMEREQD);
	
	int rtr_len = strlen(name)+strlen(numeric_name)+4;
	char *rtr = malloc(rtr_len);
	snprintf(rtr,rtr_len,"%s [%s]",name,numeric_name);

	free(name);
	free(numeric_name);
	return rtr;
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
	char *free_new_str = new_string;

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

	free(free_new_str);
	return decoded_str;
}

void
write_pid_file(const char* filename) {
	FILE *file = fopen(filename,"w");
	int pid = getpid();
	fprintf(file,"%u",pid);
	fclose(file);
}

void signal_handler(int signal) {
	syslog(LOG_INFO, "stopping");
	closelog();
	//TODO: delete pid file
	//TODO: close socket
	//close(socket);
	exit(0);
}
