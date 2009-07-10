#include <utils.h>

void daemonize()
{
        int pid;
        pid = fork();
        if (pid > 0)
        {
                //syslog(LOG_INFO,"Exiting Parent");
                exit(0);
        }
}

char* stripCRLF(char* line)
{
        char *tmp = line;

        if((tmp = strstr(line,"\r\n")) != NULL)
                *tmp = '\0';
        else if((tmp = strchr(line,'\n')) != NULL)
                *tmp = '\0';

        return line;
}

char* getpeeraddress(int socket)
{
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

char* base64_decode(char* string)
{
        //TODO: replace this with own implementation
        int b64_len = strlen(string);
        return g_base64_decode((gchar*)string,(gsize*)&b64_len);
}

