#define PORT 1337 
#define LOCAL_DOMAIN "x1598.at"
#define FQDN "zemanek.x1598.at"

#define VERSION "cherami 0.1"
#define MAX_MAIL_SIZE 52428800
#define SPOOL_FILE_DIR "/var/spool/cherami"
#define MDA "/usr/local/bin/procmail"
#define SOCK_PATH "/var/run/saslauthd/mux"
#define PID_FILE "/tmp/cherami.pid"
#define BUF_SIZ 4092 

//Word of advice: DON'T fuck with those.
#define SMTP_GREETING "220 %s ESMTP %s Ready.\r\n"
#define SMTP_EHLO "250-%s Hello %s\r\n250-SIZE %d\r\n250 AUTH LOGIN\r\n"
#define SMTP_VRFY_EXPN "252 Carrier Pigeons are sworn to secrecy! *flutter*\r\n"
#define SMTP_QUIT "221 %s closing connection\r\n"
#define SMTP_UNRECOGNIZED "500 unrecognized command\r\n"
#define SMTP_AUTH_LOGIN_USER "334 VXNlcm5hbWU6\r\n"
#define SMTP_AUTH_LOGIN_PASS "334 UGFzc3dvcmQ6\r\n"
#define SMTP_UNRECOGNIZED_AUTH "504 Unrecognized authentication type\r\n"
#define SMTP_AUTH_FAIL "501 Authentication failed\r\n"
#define SMTP_AUTH_OK "235 OK\r\n"
#define SMTP_UNEXPECTED_MAIL_FORMAT "503 Mailadress Format not as expected\r\n"
#define SMTP_OK "250 OK\r\n"
#define SMTP_OUT_OF_SEQUENCE "503 I don't _have_ to talk to you. *pout*\r\n"
#define SMTP_RELAY_FORBIDDEN "550 relay not permitted\r\n"
#define SMTP_ACCEPTED "250 Accepted\r\n" 
#define SMTP_ENTER_MESSAGE "354 Enter message, ending with \".\" on a line by itself\r\n"
