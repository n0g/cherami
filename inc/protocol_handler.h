char* extract_address(char* line);
int smtp_auth_login(const int socket);
int smtp_auth(const int socket,const char* line);
void smtp_ehlo(const int socket,const char* hostname,const char* ip);
char* smtp_mail(int socket,char* line);
char* smtp_rcpt(const int socket,const char* from, int authenticated,char* line);
int smtp_data(const int socket,const char* rcpt, char* data);
void handle_client(const int socket);
