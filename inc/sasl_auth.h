int open_socket(const char* path);
void send_string(int socket, const char* string);
char* receive_string(int socket);
int authenticate(	const char* socket_path,
			const char* username,
			const char* password,
			const char* service,
			const char* realm);
