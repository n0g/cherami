/* server functionality */
void daemonize();
void write_pid_file(const char* filename);
void signal_handler(int signal);

/* utilities */
char* getpeeraddress(int socket);
char* base64_decode(const char* string);
