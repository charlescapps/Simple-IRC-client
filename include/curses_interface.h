#ifndef MY_CURSES
#define MY_CURSES

#include <curses.h>
#include <stdlib.h>
#include <unistd.h>
#include "my_funcs.h"
#include "commands.h"

const int MAX_MSGS; 
const int MAX_LINE;
const char* title1; 
const char* title2; 
const char* default_prompt; 

void init_curses();
void draw_title(); 
void draw_prompt(); 
void draw_msgs(); 
char* curses_input(); //Deals with user's input. If user presses enter returns command string
void add_msg_to_scrn(char* msg); 
void set_new_prompt(char* new_prompt);

void print_debug(char* msg); 
void clear_chat_lines(); 



#endif
