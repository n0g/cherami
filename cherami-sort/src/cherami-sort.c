#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>

#define SPOOLDIR "/home/matthias/src/cherami/test/var/spool/cherami"
#define USERDIR "/home/matthias/src/cherami/test/etc/cherami/users"
#define MAXADLEN 64+1+255+1
#define MAXRECIPIENTS 100

char*
extractdomain(char *mailaddress) {
	char *start;

	if((start = strchr(mailaddress,'@')) != NULL) {
		start++;
	}
	
	return start;
}

char*
extractuser(char *mailaddress) {
	char *end;

	if((end = strchr(mailaddress,'@')) != NULL) {
		*end = '\0';
	}

	return mailaddress;
}

int
cpyfile(const char *src, const char *dst) {
	FILE *srcfile, *dstfile;
	char dstpath[BUFSIZ], buffer[BUFSIZ];
	snprintf(dstpath,(size_t) BUFSIZ,"%s/%s",dst,basename(src));

	/* TODO: check if dstpath exists and exit function if it does */
	srcfile = fopen(src,"r");
	dstfile = fopen(dstpath,"w");

	if(srcfile == NULL || dstfile == NULL) {
		return EXIT_FAILURE;
	}
	
	while(!feof(srcfile)) {
		size_t numb;
		numb = fread(buffer,1,BUFSIZ,srcfile);
		fwrite(buffer,1,numb,dstfile);	
	}

	return EXIT_SUCCESS;
}

int
main(int argc, char *argv[]) {
	FILE *newcom;

	while((newcom = fopen(SPOOLDIR"/newcom","r")) != NULL) {
		FILE *newmail;
		char mail[BUFSIZ], recipient[MAXADLEN];
		short rcpcount;

		/* wait for new path from fifo */
		if(fgets(mail,BUFSIZ,newcom) == NULL) {
			fprintf(stderr,"%s: Error reading from communication fifo\n",argv[0]);
			fclose(newcom);
			continue;
		}
		/* open new file */
		if((newmail = fopen(mail,"r")) == NULL) {
			fprintf(stderr,"%s: Error opening new mail\n",argv[0]);
			fclose(newcom);
			continue;
		}
		/* read all recipients */
		for(rcpcount = 0;rcpcount < 100; rcpcount++) {
			char *domain, *user;
			char path[BUFSIZ];
			/* read recipient */
			fsetpos(newmail,(rcpcount+1)*MAXADLEN);
			fgets(recipient[rcpcount],MAXADLEN,newmail);
			/* check if local recipient or relay recipient */
			domain = extractdomain(recipient[rcpcount]);	
			user = extractuser(recipient[rcpcount]);

			snprintf(path,(size_t) BUFSIZ,"%s/%s/%s",USERDIR,domain,user);
			if(access(path,F_OK) == 0) {
				if(cpyfile(mail,SPOOLDIR"/local")) {
					/* TODO: contact cherami-local */	
				}
			}
			else {
				char relaypath[BUFSIZ];
				snprintf(relaypath,(size_t) BUFSIZ,"%s/%s/%s",SPOOLDIR,"relay",domain);
				mkdir(relaypath,S_IRWXU|S_IRWXG);
				if(cpyfile(mail,relaypath)) {
					/* TODO: contact cherami-relay */
				}
			}
		}
		fclose(newmail);
		/* TODO: delete file */
		fclose(newcom);
	}
	fprintf(stderr,"%s: Couldn't not open communication fifo\n",argv[0]);
	return EXIT_SUCCESS;
}
