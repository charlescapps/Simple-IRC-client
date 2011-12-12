#ifndef _IRC_CONSTANTS
#define _IRC_CONSTANTS

#define BUFFER_SIZE 512
#define NUM_WORKERS 10
#define MAX_SERVER_MSGS 10
#define TIME_STR_SIZE 32
#define MAX_DEBUG_MSG 128

//Message when program starts
const char* WELCOME; 

//Message to user when they quit
const char* QUIT_MSG;

//Help message before entering Lobby, after which the server will dynamically supply help messages
const char* HELP_MSG;

#endif
