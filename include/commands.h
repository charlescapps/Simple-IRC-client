#ifndef _IRC_COMMANDS
#define _IRC_COMMANDS

/*
 * Constants defined in commands.c
 */

//Command so that running scripts is possible. ncurses doesn't like input coming in too fast
const char* SLEEP_CMD; 

//Commands from client
const char* CONNECT_CMD; 
const char* HELP_CMD; 
const char* MSG_CMD; 
const char* QUIT_CMD;
const char* LIST_CMD; //list names of ppl in channel
const char* LIST_ALL_CMD; //list names of ppl in channel
const char* ROOMS_CMD;
const char* JOIN_CMD;

//Commands to client
const char* PRINT_CMD; 
const char* SET_PROMPT_CMD; 
const char* FAIL_CMD; 
const char* JOIN_SUCCESS_CMD; 


#endif
