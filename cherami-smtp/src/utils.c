#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>

#include <utils.h>
#include <config.h>

/*
 * Let program fall to background. Fork 
 * process and exit parent.
 */
void
daemonize() {
        if (fork() > 0) {
                syslog(LOG_INFO,"daemonize: Exiting Parent");
                exit(EXIT_SUCCESS);
        }
}

void
write_pid_file(const char* filename) {
	FILE *file = fopen(filename,"w");
	fprintf(file,"%u",getpid());
	fclose(file);
}

void
signal_handler(int signal) {
	syslog(LOG_INFO, "stopping");
	closelog();
	if(remove(PID_FILE) == -1) {
		syslog(LOG_ERR,"Couldn't delete PID File in %s",PID_FILE);
		//TODO: close socket
		//close(socket);
	}
	exit(EXIT_SUCCESS);
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
 * Decode a base64 encoded STRING. This will NOT
 * work for anything other than a null-terminated
 * string.
 */
char*
base64_decode(const char* string) {
	char base64_table[66] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; 
	//replace all = by A so that base64 fill bytes end up being 
	//null terminating bytes in the decoded string
	char *new_string = strdup(string);
	char *p;
	while((p = strrchr(new_string,'=')) != NULL) {
		*p = 'A';
	}
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
