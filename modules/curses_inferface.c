#include "../include/curses_interface.h"

const char* title1 = "<-----------------------Welcome to Chats R Us, Copyright 2011 Charles Capps------------------------>";
const char* title2 = "****************************************************************************************************";
const char* above_prompt =
					 "****************************************************************************************************"; 
const char* blank_line = 
					 "*                                                                                                  *"; 
const char* default_prompt= "PROMPT>";  

const int MAX_MSGS = 1024; 
const int MAX_LINE = 98; //100 - 2 for borders
const int DEBUG_ROW = 10; 
const int DEBUG_COL = 101; 

//Static variables only accessible by this module
static WINDOW* screen; 
static char** ALL_MSGS; 
static int curr_num_msgs; 
static int curr_msg_row; 
static char* command_buffer; 
static char* prompt; 

void init_curses() {

	command_buffer = NULL; 
	prompt = (char*)malloc(sizeof(char)*(strlen(default_prompt) + 1)); 
	strcpy(prompt, default_prompt); 

	ALL_MSGS = (char**)malloc(sizeof(char*)*MAX_MSGS); 
	int i; 
	for (i = 0; i < MAX_MSGS; i++) 
		ALL_MSGS[i] = (char*)malloc(sizeof(char)*MAX_LINE); 

	curr_num_msgs = 0; 
	curr_msg_row = 0;

	screen = initscr(); 
	keypad(screen, TRUE);
	//meta(screen, TRUE) ;
	cbreak(); 
	noecho(); 
//	getmaxyx(screen, win_rows, win_cols); 
	clear(); 
	refresh(); 
}

void set_new_prompt(char* new_prompt) {
	if (new_prompt == NULL || strlen(new_prompt) <= 0) {
		add_msg_to_scrn("Debugging: Invalid prompt sent from server."); 
	}

	free(prompt); 
	prompt = (char*)malloc(sizeof(char)*(strlen(new_prompt)+1)); 
	strcpy(prompt, new_prompt); 
}

void add_msg_to_scrn(char* msg) {//Parse a message line-by-line and add to our list of messages
	char buffer[BUFFER_SIZE]; //Msg might be read-only, so must copy to a buffer 
	strcpy(buffer, msg); 
	trim_str(buffer); 
	//printw("Message added: '%s'", buffer); 
	char* tok = strtok(buffer, "\n"); 
	while (tok != NULL) {
		strcpy(ALL_MSGS[curr_msg_row++], tok); 
		curr_num_msgs++; 
		if (curr_msg_row >= MAX_MSGS) { //Wrap around if we've filled up message buffer
			curr_msg_row = 0; 
		}
		tok = strtok(NULL, "\n"); 
	}

	draw_msgs(); 
}	

void print_debug(char* msg) {
	int x, y; 
	getyx(screen, y, x); 

	mvaddstr(DEBUG_ROW, DEBUG_COL, msg); 
	wmove(screen, y, x); 


}

void clear_chat_lines() {
	int row; 
	for (row = 2; row < LINES - 2; row++) {
		mvaddstr(row, 0, blank_line); 
	}
}

void draw_title() {
	
	mvaddstr(0, 0, title1); 
	mvaddstr(1, 0, title2); 
}

void draw_prompt() {
	mvaddstr(LINES - 2, 0, above_prompt); 
	mvaddstr(LINES - 1, 0, prompt); 
	if (command_buffer != NULL && strlen(command_buffer) > 0) {
		mvaddstr(LINES - 1, strlen(prompt), command_buffer); 
	}
}

void draw_msgs() {

	werase(screen); //Erase screen in case some graphical glitch has occurred like typing weird chars...
	draw_title(); 
	//clear_chat_lines(); 

	const int CHAT_LINES = LINES - 4; //2 lines for title, 2 lines for prompt, LINES - 4 leftover
	int msg_row; //Variable indicating what msg we are drawing 
	int num_lines_drawn = 0; //Num lines we've drawn so far in chat window

	if (curr_num_msgs <= MAX_MSGS) { //Haven't overflowed message history and wrapped around
		msg_row = MAX(0, curr_msg_row - CHAT_LINES); 
	}
	else {//Wrap around message buffer. 
		msg_row = curr_msg_row - CHAT_LINES; 
		if (msg_row < 0)
			msg_row += MAX_MSGS; 
	}

	int screen_row = 2; //Title will take up 2 rows, so start drawing chat window at row 2 

	for (; num_lines_drawn < CHAT_LINES; screen_row++, msg_row++, num_lines_drawn++) {
		if (msg_row >= MAX_MSGS) {  //Wrap around buffer
			msg_row = 0;
		}
		if (num_lines_drawn >= curr_num_msgs) { //If we have drawn more lines than current # of messages
			mvaddstr(screen_row, 0, blank_line); 
		}
		else {
			mvaddstr(screen_row, 0, "*"); 
			mvaddstr(screen_row, 1, ALL_MSGS[msg_row]); 
			mvaddstr(screen_row, MAX_LINE + 1, "*"); 
		}

	}
	
	draw_prompt(); 

	refresh(); 
}

char* curses_input() {
	int c; 
	//Buffer for storing command user typed
	command_buffer = (char*)malloc(sizeof(char)*(MAX_LINE + 1));
	memset(command_buffer, ' ', MAX_LINE); 
	command_buffer[0] = '\0'; 

	char blanks[MAX_LINE]; 
	memset(blanks, ' ', MAX_LINE); 
	blanks[MAX_LINE - 1] = '\0'; 

	int cmd_i = 0; 
	int x, y; 
	int max_index; 

	while (true) {
		c = wgetch(screen); 
		if (c == KEY_BACKSPACE) {
			getyx(screen, y, x); 
			wmove(screen, y, MAX(strlen(prompt), x - 1)); 
			delch(); 
			cmd_i = MAX(0, cmd_i - 1); 
			command_buffer[cmd_i] = '\0'; 
		}
		else if (c == '\n') {
			mvaddstr(LINES - 1, strlen(prompt), blanks ); 
			wmove(screen, LINES - 1, strlen(prompt)); 
			return command_buffer;  
		}
		else if (c == KEY_UP) {
		
		}
		else {
			max_index = MAX_LINE - strlen(prompt) - 10; //Allow for chars taken up by "Blah says:" 
			if (cmd_i < max_index) {
				printw("%c", c); 
			}
			else {
				getyx(screen, y, x); 
				wmove(screen, y, MAX(strlen(prompt), x - 1)); 
				printw("%c", c); 
			}
			command_buffer[cmd_i++] = c;
			cmd_i = MIN(cmd_i, max_index); 	
			command_buffer[cmd_i] = '\0'; 
		}
		refresh(); 
	}

}
