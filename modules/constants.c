#include "../include/constants.h"

//Message when program starts
const char* WELCOME = "<----------------Welcome to Chats R Us.--------------->\n"
						"To start chatting, type +CONNECT <nick name>\n"
						"To get help, type +HELP.\n"
						"To quit, type +QUIT.\n"
					  "<----------------------------------------------------->\n"; 

//Message to user when they quit
const char* QUIT_MSG = "<-----------Thank you for using Chats R Us.----------->\n"
					   "                   Have a nice day.\n"
					   "<----------------------------------------------------->\n"; 

//Help message before entering Lobby, after which the server will dynamically supply help messages
const char* HELP_MSG = "Must connect to server by giving a nick:\n"
							"\t+CONNECT <nick> : connect to server with given nick\n"
							"\t+QUIT : Close connection and quit program\n"
							"\t+HELP : Display help message for your current context\n \n";
