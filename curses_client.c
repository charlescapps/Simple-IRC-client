#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "errno.h"
#include "sys/socket.h"
#include "sys/types.h"
#include "netdb.h"
#include "stdbool.h"
#include "pthread.h"
#include <curses.h>

#include "include/my_funcs.h"
#include "include/commands.h"
#include "include/curses_interface.h"

const char* WELCOME = "<----------------Welcome to Chats R Us.--------------->\n"
						"To start chatting, type +CONNECT <nick name>\n"
						"To get help, type +HELP.\n"
						"To quit, type +QUIT.\n"
					  "<----------------------------------------------------->\n"; 

const char* QUIT_MSG = "<-----------Thank you for using Chats R Us.----------->\n"
					   "                   Have a nice day.\n"
					   "<----------------------------------------------------->\n"; 

//Help message before entering Lobby, after which the server will dynamically give help messages
const char* HELP_MSG = "Must connect to server by giving a nick:\n"
							"\t+CONNECT <nick> : connect to server with given nick\n"
							"\t+QUIT : Close connection and quit program\n"
							"\t+HELP : Display help message for your current context\n";

pthread_t receive_thread; 


int connect_to_server(char* host, char* socket_str); //Returns socket to server, of -1 if it failed
void command_loop(int server_socket); 
void act_on_command(char* server_msg); 
void send_outgoing_command(char* user_msg, int server_sock);
void receive_loop(void* sock);

int main(int argc, char** argv) {

	if (argc != 3) {
		fprintf(stderr, "Usage: run_client <host_name> <port>\n"); 
		exit(EXIT_FAILURE); 
	}	

	int server_socket = -1; 
	int error = -1; 
	char* host = argv[1];
	char* port = argv[2]; 

	init_curses(); 
	draw_msgs(); 

	server_socket = connect_to_server(host, port); 

	error = pthread_create(&receive_thread, NULL, (void*(*)(void*)) receive_loop, (void*) &server_socket); 

	if (error != 0) {
		endwin(); 
		printf("Error creating pthread. Exiting program.\n"); 
		exit(EXIT_FAILURE); 
	}	

	command_loop(server_socket); 

	endwin(); 

	return 0;
}

void receive_loop(void* sock) {
	char buffer[BUFFER_SIZE]; 	
	int server_socket = *((int*)sock); 
	int bytes_read = -1; 

	while (true) {

		if ( (bytes_read = read(server_socket, buffer, BUFFER_SIZE)) < 0) { 
			add_msg_to_scrn("Error reading from server's socket. Re-trying."); 
			continue; 
		}
		else if (bytes_read == 0) { 
			endwin(); 
			printf("%s\nEnd of file reached for server socket. Server must have crashed. Exiting program.\n", QUIT_MSG); 
			exit(EXIT_FAILURE); 
		}

		act_on_command(buffer);
	}
}

void command_loop(int server_socket) {

	char* next_cmd = NULL; 

	while (true) {

		next_cmd = curses_input(); 

		send_outgoing_command(next_cmd, server_socket); 

		free(next_cmd); 
		
	}
	endwin(); 
	printf("Error: %s\n", strerror(errno)); 
	exit(EXIT_FAILURE); 
}

int connect_to_server(char* host_str, char* socket_str){ //Returns socket to server, of -1 if it failed
	struct addrinfo connect_to; 
	struct addrinfo *result, *ptr; 
	int get_addr_error, server_socket; 

	memset(&connect_to, 0, sizeof(struct addrinfo)); 

	connect_to.ai_family = AF_INET;  //IPv4
	connect_to.ai_socktype = SOCK_STREAM; 
	connect_to.ai_flags = 0; 
	connect_to.ai_protocol = 0; 

	get_addr_error = getaddrinfo(host_str, socket_str, &connect_to, &result); 

	if (get_addr_error != 0) {
		endwin(); 
		fprintf(stderr, "%s\nUnable to connect to server: %s\n", QUIT_MSG, gai_strerror(get_addr_error));  
		exit(EXIT_FAILURE); 
	}

	for (ptr = result; ptr != NULL; ptr = ptr -> ai_next) {
		server_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); 
		if (server_socket < 0) 
			continue; //failed to get this socket

		//If we succeeded in connecting to server, stop trying addresses
		if (connect(server_socket, ptr->ai_addr, ptr->ai_addrlen) != -1) 
			break; 

		close(server_socket); //Close the socket if we fail to connect
	}

	if (ptr == NULL) {
		fprintf(stderr, "All addresses returned by getaddrinfo failed to connect!\n"); 
		endwin(); 
		exit(EXIT_FAILURE); 
	}

	freeaddrinfo(result); //According to man page, this is no longer used after connecting

	return server_socket; 

}

void send_outgoing_command(char* user_msg, int server_sock) {
	int bytes_sent = -1; 
  	char to_send[BUFFER_SIZE];
	char msg_copy[BUFFER_SIZE]; //strtok modifies the data in the argument. So use this copy. 
	strcpy(msg_copy, user_msg); 

	trim_str(user_msg);
	char* token = strtok(user_msg, " ");

	if (token == NULL || strlen(token) <= 0) { //Do nothing if user sends bad / empty command
		return; 
	}

	if (strcmp(token, MSG_CMD) == 0 || strcmp(token, CONNECT_CMD) == 0 || strcmp(token, HELP_CMD) == 0) { //If user included '+MSG', '+CONNECT' just send as-is 
		strcpy(to_send, msg_copy); 
	}	
	else if (strcmp(token, QUIT_CMD) == 0) {
		endwin(); 
		printf("%s", QUIT_MSG); 
		write(server_sock, msg_copy, strlen(msg_copy) + 1);
		close(server_sock); 
		exit(EXIT_SUCCESS); 
	}
	else { //Else we need to add +MSG so server knows what command we're using
		sprintf(to_send, "%s %s", MSG_CMD, msg_copy); 
	}

//	printf("ACTUAL COMMAND BEING SENT: %s\n", to_send); 

	if ((bytes_sent = write(server_sock, to_send, strlen(to_send) + 1)) < 0) { 
		add_msg_to_scrn("Error writing to server socket. Attempting again.\n"); 
	}
	else if (bytes_sent == 0) {
		endwin(); 
		printf("Failed to write bytes to server socket. Exiting program...\n"); 
		exit(EXIT_FAILURE); 
	}

}

void act_on_command(char* server_msg) {
	char* tok1; 
	char* msg; 
	char backup[BUFFER_SIZE]; 

	strcpy(backup, server_msg); 

	tok1 = strtok(server_msg, " "); 

	if (strcmp(tok1, PRINT_CMD) == 0) {
		msg = (char*)(backup + strlen(PRINT_CMD) + 1); 
		add_msg_to_scrn(msg); 
	}	
	else if (strcmp(tok1, FAIL_CMD) == 0) {
		add_msg_to_scrn((char*)HELP_MSG); 
	}
	else if (strcmp(tok1, SET_PROMPT_CMD) == 0) {
		char* new_prompt = backup + (strlen(SET_PROMPT_CMD) + 1); 
		set_new_prompt(new_prompt); 
		draw_prompt(); 
		refresh(); 
	}
	else {
		add_msg_to_scrn("Received non-standard message from server:"); 
		add_msg_to_scrn(backup); 
	}

}

