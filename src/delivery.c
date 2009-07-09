#include <string.h>
#include <stdio.h>

#include <config.h>
#include <delivery.h>

int deliver_mail(char* ip,char* from,char* rcpt,char* data,int datalen)
{
        char* domain = strchr(rcpt,'@')+1;
        printf("IP-Address: %s\nFrom: %s\nRecipient: %s\nDomain: %s\nData:\n%s\n",ip,from,rcpt,domain,data);

        if(strcmp(domain,DOMAIN) == 0)
        {
                //local delivery
                return deliver_local(ip,from,rcpt,data,datalen);
        }
        else
        {
                //relay
                return deliver_relay(ip,domain,from,rcpt,data,datalen);
        }
}

int deliver_local(char* ip,char* from,char* rcpt,char* data,int datalen)
{
        printf("local delivery\n");
        *(strchr(rcpt,'@')) = '\0';
        printf("user: %s\n",rcpt);
        return 0;
}

int deliver_relay(char* ip,char* targetdomain,char* from,char* rcpt,char* data,int datalen)
{
        printf("relay delivery\n");
        //get mailserver for targetdomain
        //open socket to mailserver
        //protocolhandling
        return 0;
}

