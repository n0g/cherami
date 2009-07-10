void sasl_send_str(int socket, const char* string);
char* sasl_receive_str(int socket);
int sasl_auth(	const char* socket_path,
			const char* username,
			const char* password,
			const char* service,
			const char* realm);
