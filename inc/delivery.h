int deliver_mail(char* ip,char* from,char* rcpt,char* data,int datalen);
int deliver_local(char* ip,char* from,char* rcpt,char* data,int datalen);
int deliver_relay(char* ip,char* targetdomain,char* from,char* rcpt,char* data,int datalen);
