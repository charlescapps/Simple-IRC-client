#ifndef MY_CURSES
#define MY_CURSES

#include <curses.h>
#include <stdlib.h>
#include <unistd.h>
#include "misc.h"
#include "commands.h"
#include "constants.h"

/*
 * Manages ncurses functionality. Stores previous lines of messages for chat history. 
 */

//Defined in curses_interface.c
const char* title1; 
const char* title2; 
const char* default_prompt; 

//Init ncurses settings
void init_curses(bool);

//Helper function to draw title at top
void draw_title(void); 

//Re-draw prompt
void draw_prompt(void);

//Re-draw messages in chat window
void draw_msgs(void); 

//Processes user's input char-by-char. If user presses enter returns command string
char* curses_input(bool); 

//Adds a line (or multiple lines separated by \n) to the chat window
void add_msg_to_scrn(char* msg); 

//Adds a single line
void add_line_to_screen(char* line);

//Set new prompt, response to +PROMPT command
void set_new_prompt(char* new_prompt);

//Removes all data in chat window
void clear_chat_history(void);

//Scroll up chat window by 1 line. PAGE_UP key.
void scroll_up_one(void); 

//Scroll down chat window by 1 line. PAGE_UP key.
void scroll_down_one(void); 
#endif
