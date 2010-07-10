/* protocol_handler.c
 *
 * Implements the server-side of the SMTP protocol.
 * 
 * Author: Matthias Fassl <mf@x1598.at>
 * Date: 2009-07-09
 * License: MIT (see enclosed LICENSE file for details)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <syslog.h>

#include <delivery.h>
#include <protocol_handler.h>
#include <config.h>
#include <sasl_auth.h>
#include <utils.h>

/*
4.5.1 Minimum Implementation

   Any system that includes an SMTP server supporting mail relaying or
   delivery MUST support the reserved mailbox "postmaster" as a case-
   insensitive local name.  This postmaster address is not strictly
   necessary if the server always returns 554 on connection opening (as
   described in section 3.1).  The requirement to accept mail for
   postmaster implies that RCPT commands which specify a mailbox for
   postmaster at any of the domains for which the SMTP server provides
   mail service, as well as the special case of "RCPT TO:<Postmaster>"
   (with no domain specification), MUST be supported.

   SMTP systems are expected to make every reasonable effort to accept
   mail directed to Postmaster from any other system on the Internet.
   In extreme cases --such as to contain a denial of service attack or
   other breach of security-- an SMTP server may block mail directed to
   Postmaster.  However, such arrangements SHOULD be narrowly tailored
   so as to avoid blocking messages which are not part of such attacks.
*/
void
handle_client(const int socket) {
	/* TODO: meta data structure
 		from, recipients, ip, authentication, mailpath
	*/
        char *from = NULL, *rcpt = NULL, *ip;
	int authenticated = 0;

	char line[MAX_LINE_LENGTH];

	ip = getpeeraddress(socket);
	syslog(LOG_INFO, "Accepted SMTP Connection from %s",ip); 

        /* send greeting */
	tcp_send_str(socket,SMTP_GREETING,FQDN,VERSION);

        while(tcp_receive_line(socket,line,MAX_LINE_LEN)) {
		if(line[0] >= 'a') {
			line[0] -= ' ';
		}	
		switch(line[0]) {
			/* EHLO */
			case 'E':
			case 'H':
				smtp_ehlo(socket,hostname,ip);
				break;
			/* Authentication*/
			case 'A':
				authenticated = smtp_auth(socket,line);
				break;
			/* Mail from */
			case 'M':
				from = smtp_mail(socket,line);
				break;
			/* Recipient OR RESET */
			case 'R':
				if(line[1] == 'C' || line[1] == 'c') {
					//TODO: what about more than 1 recipient? array of recipients?
					rcpt = smtp_rcpt(socket,from,authenticated,line);
				}
				else {
					//TODO: reset all parameters of this mail transaction
					from = NULL;
					rcpt = NULL;
					tcp_send_str(socket,SMTP_OK);
				}	
				break;
			/* verify */
			case 'V':
				syslog(LOG_INFO, "%s sent a VRFY command, he could be a spambot",ip);
				tcp_send_str(socket, SMTP_VRFY_EXPN);
				break;
			/* data */
			case 'D':
				//TODO: add lines to the email header
				syslog(LOG_INFO, "Accepted Email from %s to %s delivered by %s",from,rcpt,ip);
                       		deliver_mail(ip,from,rcpt,data,data_cnt);
				tcp_send_str(socket, SMTP_OK);
				break;
			/* no operation */
			case 'N':
				tcp_send_str(socket,SMTP_OK);
				break;
			/* quit */
			case 'Q':
				tcp_send_str(socket, SMTP_QUIT, hostname);
				break
			default:
				syslog(LOG_INFO,"%s sent unrecognized SMTP command %s",ip,line);
				tcp_send_str(socket, SMTP_UNRECOGNIZED);
				
		}
        }

        shutdown(socket,2);
        exit(EXIT_SUCCESS);
}

/*
 * Extract the data betwenn first '<' and first '>'.
 * Needed in smtp_rcpt and smtp_mail to extract the
 * mail address from input lines.
 */
char*
extract_address(char* line) {
        char* address;
	if((address = strchr(line,'<')) == NULL)
		return NULL;
	address++;

	if(strchr(address, '>') == NULL)
		return NULL;	
	//TODO: don't write into that string
        *(strchr(address,'>')) = '\0';

        return address;
}

int
smtp_auth_login(const int socket) {
	char *recv_line;
	char *username,*password;

	//send username request (base64 encode)
	tcp_send_str(socket, SMTP_AUTH_LOGIN_USER);
	//extract username (base64 decode it)
	recv_line = (char*) tcp_receive_line(socket);
	username = base64_decode(recv_line);
	syslog(LOG_INFO, "%s tries to authenticate", username);
	free(recv_line);
	//send password request (base64 encoded)
	tcp_send_str(socket, SMTP_AUTH_LOGIN_PASS);
	//extract password (base64 decode it)
	recv_line = (char*) tcp_receive_line(socket);
	password = base64_decode(recv_line);
	free(recv_line);
	//call sasl auth function
	if(!sasl_auth(SOCK_PATH,username,password,"smtp",""))
		return 1;
	else 
		return 0;
}

int
smtp_auth(const int socket,const char* line) {
	int authenticated = 0;
	//test for LOGIN or PLAIN type
	char* type = strchr(line,' ')+1;
	char *up_type = str_toupper(type);

	if(!strncmp(up_type,"LOGIN",5)) {
		authenticated = smtp_auth_login(socket);
	}
	else {
		tcp_send_str(socket, SMTP_UNRECOGNIZED_AUTH);
		free(up_type);
		return authenticated;
	}

	if(authenticated)
		tcp_send_str(socket, SMTP_AUTH_OK);
	else
		tcp_send_str(socket, SMTP_AUTH_FAIL);

	free(up_type);
	return authenticated;
}

void
smtp_ehlo(const int socket,const char* hostname,const char* ip) {
	//TODO: dynamically send available extensions
	tcp_send_str(socket, SMTP_EHLO, hostname, ip, MAX_MAIL_SIZE);
}

char*
smtp_mail(const int socket,char* line) {
	char *from, *tmp;
	//check if mail address is well formed
	//TODO: check for at sign etc - external function check_mail_address() ?
	if((tmp = extract_address(line)) == NULL) {
		tcp_send_str(socket, SMTP_UNEXPECTED_MAIL_FORMAT);
		return NULL;
	}
	from = malloc(strlen(tmp)+1);
	strncpy(from,tmp,strlen(tmp));

	tcp_send_str(socket, SMTP_OK);

	return from;
}

char*
smtp_rcpt(const int socket,const char* from,int authenticated, char* line) {
	char *rcpt, *tmp;
	if((tmp = extract_address(line)) == NULL) {
		tcp_send_str(socket, SMTP_UNEXPECTED_MAIL_FORMAT);
		return NULL;
	}
	rcpt = malloc(strlen(tmp)+1);
	strncpy(rcpt,tmp,strlen(tmp));

	if(from == NULL) {
		tcp_send_str(socket, SMTP_OUT_OF_SEQUENCE);
		return rcpt;
	}

	//TODO: well formed? check for at sign etc - external function check_mail_address() ?
	if(strchr(rcpt,'@') == NULL) {
		return NULL;
	}

	char* domain = strchr(rcpt,'@')+1;
	if(strncmp(domain,LOCAL_DOMAIN,strlen(LOCAL_DOMAIN)) && !authenticated) {
		tcp_send_str(socket, SMTP_RELAY_FORBIDDEN);
		return NULL;
	}

	tcp_send_str(socket, SMTP_ACCEPTED);
	return rcpt;
}

int 
smtp_data(const int socket, const char* rcpt, char* data) {
/*
4.5.2 Transparency

   Without some provision for data transparency, the character sequence
   "<CRLF>.<CRLF>" ends the mail text and cannot be sent by the user.
   In general, users are not aware of such "forbidden" sequences.  To
   allow all user composed text to be transmitted transparently, the
   following procedures are used:

   -  Before sending a line of mail text, the SMTP client checks the
      first character of the line.  If it is a period, one additional
      period is inserted at the beginning of the line.

   -  When a line of mail text is received by the SMTP server, it checks
      the line.  If the line is composed of a single period, it is
      treated as the end of mail indicator.  If the first character is a
      period and there are other characters on the line, the first
      character is deleted.

   The mail data may contain any of the 128 ASCII characters.  All
   characters are to be delivered to the recipient's mailbox, including
   spaces, vertical and horizontal tabs, and other control characters.
   If the transmission channel provides an 8-bit byte (octet) data
   stream, the 7-bit ASCII codes are transmitted right justified in the
   octets, with the high order bits cleared to zero.  See 3.7 for
   special treatment of these conditions in SMTP systems serving a relay
   function.

   In some systems it may be necessary to transform the data as it is
   received and stored.  This may be necessary for hosts that use a
   different character set than ASCII as their local character set, that
   store data in records rather than strings, or which use special
   character sequences as delimiters inside mailboxes.  If such
   transformations are necessary, they MUST be reversible, especially if
   they are applied to mail being relayed.

*/
	//check if recipient is already known, otherwise smtp commands out of sequence
	if(rcpt ==  NULL) {
		tcp_send_str(socket, SMTP_OUT_OF_SEQUENCE);
		return -1;
	}
	tcp_send_str(socket, SMTP_ENTER_MESSAGE);

	//receive data until <CRLF>.<CRLF>
	int bytes_recv, data_size = BUF_SIZ+1, data_cnt = 0;
	char buffer[data_size], *tmp;

	data = malloc(data_size);
	tmp = data;
	//TODO: define maximum size of data
	//TODO: what about the transparency feature?
	do {
		memset(buffer,0,BUF_SIZ);
		bytes_recv = recv(socket,buffer,BUF_SIZ,0);
		data_cnt += bytes_recv;
		if(bytes_recv < 1)
			break;
		if(data_size < data_cnt) {
			data = realloc(data,data_size*2);
			data_size *= 2;
		}
		memcpy(tmp,buffer,bytes_recv);
		tmp+=bytes_recv;
	} while(strstr(data,"\n.\n") == NULL && strstr(data,"\r\n.\r\n") == NULL);

	return data_cnt;
}
