#ifndef _IRC_COMMANDS
#define _IRC_COMMANDS

const int BUFFER_SIZE; 

//Commands from client
const char* CONNECT_CMD; 
const char* HELP_CMD; 
const char* MSG_CMD; 
const char* QUIT_CMD;
const char* LIST_CMD; //list names of ppl in channel
const char* ROOMS_CMD;

//Commands to client
const char* PRINT_CMD; 
const char* SET_PROMPT_CMD; 
const char* FAIL_CMD; 


#endif
