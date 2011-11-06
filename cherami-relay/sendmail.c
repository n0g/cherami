/* simple email sender
 * ~~~~~~~~~~~~~~~~~~~
 *
 * author: Matthias Fassl
 * version: 2010-07-06
 * license: MIT
 */

/* TODO: 	bailout function
 * 		error handling
 * 		better resolver library
 */	
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>

char* getMailserver(const char* domain) {
	int len;
	char answer[NS_PACKETSZ];
	char *mailserver = malloc(NS_PACKETSZ);
	ns_msg msg;
	ns_rr rr;

	if(res_init() == -1) {
		return NULL;
	}
	len = res_query(domain,C_ANY,T_MX,answer,NS_PACKETSZ);
	if(len == -1) {
		return NULL;
	}
	/* return first MX record */	
	ns_initparse(answer,len,&msg);
	ns_parserr(&msg,ns_s_an,0,&rr);
	dn_expand(ns_msg_base(msg),ns_msg_base(msg)+ns_msg_size(msg),ns_rr_rdata(rr)+NS_INT16SZ,mailserver,NS_PACKETSZ);
	return mailserver; 
}

int smtp_sendGreeting(int socket) {
	char buf[1024];
	char hostname[1024];
	gethostname(hostname,1024);
	snprintf(buf,1024,"EHLO %s\r\n",hostname);
	write(socket,buf,7+strlen(hostname));
}

int smtp_sendFrom(int socket,const char *mailaddress) {
	char buf[1024];
	snprintf(buf,1024,"MAIL FROM:<%s>\r\n",mailaddress);
	write(socket,buf,14+strlen(mailaddress));
}

int smtp_sendRecipient(int socket, const char *mailaddress) {
	char buf[1024];
	snprintf(buf,1024,"RCPT TO:<%s>\r\n",mailaddress);
	write(socket,buf,12+strlen(mailaddress));
}

int smtp_sendDataRequest(int socket) {
	write(socket,"DATA\r\n",6);
}

int smtp_sendData(int socket) {
	char buf[1024];
	int len;
	do {
		len = read(0,buf,1024);
		write(socket,buf,len);
		memset(buf,0,1024);

	} while(len == 1024);

	write(socket,"\r\n.\r\n",5);
}

int smtp_parseResponse(int socket,int showResult) {
	char buf[1024];
	memset(&buf,0,1024);
	read(socket,buf,1024);
	if(buf[0] == '5') {
		fprintf(stderr,"%s\n",strchr(buf,' '));
		exit(EXIT_FAILURE);
	}
	if(showResult != 0) {
		printf("%s\n",strchr(buf,' '));
	}
}

int main(int argc, char *argv[]) {
	int sfd;
	char *mailserver, *domain, *rcpt, *from;
	struct addrinfo hints;
	struct addrinfo *result;
	
	/* parse arguments */
	if(argc != 3) {
		fprintf(stderr,"USAGE: %s FROM RECIPIENT\n",argv[0]);
		return EXIT_FAILURE;
	}
	from = argv[1];
	rcpt = argv[2];
	/* check input */
	domain = strrchr(rcpt,'@');
	if(domain == NULL) {
		fprintf(stderr,"%s: incorrect recipient mail address\n",argv[0]);
		return EXIT_FAILURE;
	}
	domain++;
	/* find address of destination mail server */
	mailserver = getMailserver(domain);
	if(mailserver == NULL) {
		fprintf(stderr,"%s: couldn't get mailserver for recipient address\n",argv[0]);
		mailserver = domain;
	}
		
	/* connect */
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(mailserver,"smtp",&hints,&result) != 0){
		fprintf(stderr,"%s: non resolvable mail server address\n",argv[0]);
		return EXIT_FAILURE;
	}
	if((sfd = socket(result->ai_family,result->ai_socktype,result->ai_protocol)) == -1) {
		fprintf(stderr,"%s: couldn't open socket\n",argv[0]);
		return EXIT_FAILURE;
	}
	if(connect(sfd, result->ai_addr,result->ai_addrlen) == -1) {
		fprintf(stderr,"%s: couldn't connect to mail exchange\n",argv[0]);
		return EXIT_FAILURE;
	}
	/* protocol */
	smtp_parseResponse(sfd,1);
	smtp_sendGreeting(sfd);
	smtp_parseResponse(sfd,0);
	smtp_sendFrom(sfd,from);
	smtp_parseResponse(sfd,0);
	smtp_sendRecipient(sfd,rcpt);
	smtp_parseResponse(sfd,0);
	smtp_sendDataRequest(sfd);
	smtp_parseResponse(sfd,0);
	smtp_sendData(sfd);
	smtp_parseResponse(sfd,1);
	/* disconnect & free resources */
	close(sfd);
	if(mailserver != domain) {
		free(mailserver);
	}
	freeaddrinfo(result);
	return EXIT_SUCCESS;
}
