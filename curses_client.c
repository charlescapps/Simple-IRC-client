#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>
#include <curses.h>
#include <fcntl.h>

#include "include/misc.h"
#include "include/commands.h"
#include "include/constants.h"
#include "include/curses_interface.h"

static const char* LOG_FILE = ".client_log"; 
static int LOG_FD; 
static const int SLEEP_TIME_US = 2000; 
static pthread_t receive_thread; 
static bool basic_input; 

int connect_to_server(char* host, char* socket_str); //Returns socket to server, of -1 if it failed
void command_loop(int server_socket); 
void act_on_command(char* server_msg); 
void send_outgoing_command(char* user_msg, int server_sock);
void receive_loop(void* sock);

char* usage = "Need 2 or 3 arguments.\nUsage: run_client <host_name> <port> [--stdin]\n"
				"\t<host_name>: name of host running SCoT server (required)\n"
				"\t<port>: port on host (required)\n" 
				"\t[--stdin]: simple input instead of ncurses char-by-char (optional)\n";

int main(int argc, char** argv) {

	if (argc != 3 && argc != 4) {
		fprintf(stderr, "%s\n", usage); 
		exit(EXIT_FAILURE); 
	}	

	basic_input = false; 

	if (argc == 4 && strcmp(argv[3], "--stdin") == 0) {
		basic_input = true; 
	}
	else if (argc == 4) { //Gave invalid final argument
		fprintf(stderr, "Invalid arg: '%s'\n%s", argv[3], usage); 
		exit(EXIT_FAILURE); 
	}

	int server_socket = -1; 
	int error = -1; 
	char* host = argv[1];
	char* port = argv[2]; 

	init_curses(basic_input); //Init window for ncurses interface
	draw_msgs(); //Draw empty chat window

	server_socket = connect_to_server(host, port); 

	if (server_socket < 0) {
		endwin(); 
		printf("Error connecting to server %s on socket %s\n", host, port); 
		exit(EXIT_FAILURE); 
	}

	if ((LOG_FD = open(LOG_FILE, O_WRONLY|O_APPEND|O_CREAT, S_IRWXU)) < 0) {
		printf("Error opening log file: %s\n", strerror(errno)); 
	}	

	error = pthread_create(&(receive_thread), NULL, (void*(*)(void*)) receive_loop, (void*) &server_socket); 

	if (error != 0) {
		endwin(); 
		pthread_cancel(receive_thread); 
		printf("Error creating pthread. Exiting program.\n"); 
		exit(EXIT_FAILURE); 
	}	

	command_loop(server_socket); 

	pthread_cancel(receive_thread); 

	endwin(); //Shouldn't ever reach this code. But, close window if this happens. 

	return 0;
}

void receive_loop(void* sock) {
	char buffer[BUFFER_SIZE]; 
	char debug_msg[BUFFER_SIZE]; 
	char* server_msgs[MAX_SERVER_MSGS];
	int server_socket = *((int*)sock); 
	int bytes_read = -1, i = 0, pos = 0, msg_num = 0; 

	while (true) {
		//Read message from server
		bytes_read = recv(server_socket, buffer, BUFFER_SIZE, 0); 

		//Error check
		if (bytes_read < 0) {	
			add_msg_to_scrn("Error reading from server's socket. Re-trying."); 
			continue; 
		}
		else if (bytes_read == 0) { 
			endwin(); 
			printf("%s\nEnd of file reached for server socket. Server must be down. Exiting program.\n", QUIT_MSG); 
			pthread_exit(NULL); 
		}

		//Parse into separate messages:
		pos = msg_num = 0; 
		while (msg_num < MAX_SERVER_MSGS && pos < bytes_read) {

			server_msgs[msg_num] = buffer + pos; 

			//Record every message received by the client to the log file.
			sprintf(debug_msg, "[Server] : Message # %d: '%s'\n", msg_num, server_msgs[msg_num]); 
			write(LOG_FD, debug_msg, strlen(debug_msg)+1); 

			pos += (strlen(server_msgs[msg_num]) + 1); 
			msg_num ++; 
		}

		for (i = 0; i < msg_num; i++) {
			act_on_command(server_msgs[i]); 
		}

	}
}

void command_loop(int server_socket) {

	char* next_cmd = NULL; 

	while (true) {

		next_cmd = curses_input(basic_input); 

		if (next_cmd) {
			send_outgoing_command(next_cmd, server_socket); 
			free(next_cmd);
		}	
		
	}
	endwin(); 
	printf("Error: %s\n", strerror(errno)); 
	pthread_cancel(receive_thread); 
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
		endwin(); 
		fprintf(stderr, "%s\nUnable to connect to server: %s\nAll addresses returned by getaddrinfo failed to connect!\n", QUIT_MSG, gai_strerror(get_addr_error)); 
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

	if (strcmp(token, MSG_CMD) == 0 || strcmp(token, JOIN_CMD) == 0 || 
		strcmp(token, LIST_CMD) == 0 || strcmp(token, CONNECT_CMD) == 0 || 
		strcmp(token, HELP_CMD) == 0 || strcmp(token, ROOMS_CMD) == 0 || //If user typed a valid command, send as-is 
		strcmp(token, LIST_ALL_CMD) == 0 ) { //If user typed a valid command, send as-is 
		strcpy(to_send, msg_copy); 
	}	
	else if (strcmp(token, QUIT_CMD) == 0) { //Quit command: send quit message to notify server, then exit program
		endwin(); 
		printf("%s", QUIT_MSG); 
		send(server_sock, msg_copy, strlen(msg_copy) + 1, MSG_MORE);
		close(server_sock); 
		exit(EXIT_SUCCESS); 
	}
	else if (strcmp(token, SLEEP_CMD) == 0) {
		usleep(500); 
	}
	else { //Else we assume client is just chatting, prepend +MSG to string
		sprintf(to_send, "%s %s", MSG_CMD, msg_copy); 
	}

	if ((bytes_sent = send(server_sock, to_send, strlen(to_send) + 1, MSG_MORE)) < 0) { 
		add_msg_to_scrn("Error writing to server socket. Attempting again.\n"); 
	}
	else if (bytes_sent == 0) {
		endwin(); 
		printf("Failed to write bytes to server socket. Exiting program...\n"); 
		exit(EXIT_FAILURE); 
	}

}

void act_on_command(char* server_msg) {
	char debug_msg[MAX_DEBUG_MSG]; 	
	char* original_msg = (char*)malloc(strlen(server_msg)+1); //strtok modifies original string, so first copy into a backup buffer
	char* tok1;
	char* tok2; 

	strcpy(original_msg, server_msg); 

	tok1 = strtok(server_msg, " "); 

	if (strcmp(tok1, PRINT_CMD) == 0) { //+PRINT: Print message received from server.
		tok2 = strtok(NULL, " "); 
		if (tok2 == NULL) {
			sprintf(debug_msg, "Empty print cmd received!\n"); 
			write(LOG_FD, debug_msg, strlen(debug_msg)+1); 
		}
		else {
			add_msg_to_scrn(original_msg + (strlen(PRINT_CMD) + 1));
		}	
	}	
	else if (strcmp(tok1, FAIL_CMD) == 0) { //+FAIL: Print help message
		add_msg_to_scrn((char*)HELP_MSG); 
	}
	else if (strcmp(tok1, SET_PROMPT_CMD) == 0) { //+PROMPT: set new prompt
		tok2 = strtok(NULL, " "); 
		if (tok2 == NULL) {
			sprintf(debug_msg, "Empty SET_PROMPT command received!"); 
			write(LOG_FD, debug_msg, strlen(debug_msg)+1); 
		}
		else {
			set_new_prompt(original_msg+(strlen(SET_PROMPT_CMD) + 1)); 
		}
	}
	else if (strcmp(tok1, JOIN_SUCCESS_CMD) == 0) {
		clear_chat_history(); 
	}
	else {
		add_msg_to_scrn("Received non-standard message from server:"); 
		add_msg_to_scrn(original_msg); 
	}

	free(original_msg); 

}

