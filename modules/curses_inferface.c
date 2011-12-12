#include "../include/curses_interface.h"

const char* title1 = "<-----------------------Welcome to Chats R Us, Copyright 2011 Charles Capps------------------------>";
const char* title2 = "****************************************************************************************************";
const char* above_prompt =
					 "****************************************************************************************************"; 
const char* blank_line = 
					 "*                                                                                                  *"; 
const char* default_prompt= "PROMPT>";  

static const int PAGE_UP_KEY = 73; 

static const int MAX_MSGS = 1024; 
static const int MAX_LINE = 98; //100 - 2 for borders

//Static variables only accessible by this module
static WINDOW* screen; 
static char** ALL_MSGS; 
static int curr_num_msgs; //Total # of msgs that have been output to chat room
static int curr_msg_row; //Current row of ALL_MSGS where next msg will be written
static int draw_start_row; //Current row where drawing will start
static char* command_buffer; 
static char* prompt; 

void init_curses(bool basic_input) {

	command_buffer = NULL; 
	prompt = (char*)malloc(sizeof(char)*(strlen(default_prompt) + 1)); 
	strcpy(prompt, default_prompt); 

	ALL_MSGS = (char**)malloc(sizeof(char*)*MAX_MSGS); 
	int i; 
	for (i = 0; i < MAX_MSGS; i++) 
		ALL_MSGS[i] = (char*)malloc(sizeof(char)*MAX_LINE); 

	curr_num_msgs = 0; 
	curr_msg_row = 0;
	draw_start_row = 0; 

	screen = initscr(); 
	if (!basic_input) {
		keypad(screen, TRUE); //enables getting keys like KEY_UP
		cbreak(); 
		noecho(); 
	}
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
	draw_prompt(); 
	refresh(); 
}

void add_line_to_screen(char* line) {
	strcpy(ALL_MSGS[curr_msg_row++], line); 
	curr_num_msgs++; 
	if (curr_msg_row >= MAX_MSGS) { //Wrap around if we've filled up message buffer
		curr_msg_row = 0; 
	}
	
	scroll_down_one(); 

}	

void add_msg_to_scrn(char* msg) {//Parse a message line-by-line and add to our list of messages
	char* buffer= (char*)malloc(strlen(msg) + 1); //Msg might be read-only, so must copy to a buffer 
	strcpy(buffer, msg); 

	char* tok = strtok(buffer, "\n"); 
	while (tok != NULL) {
		add_line_to_screen(tok); 
		tok = strtok(NULL, "\n"); 
	}

	free(buffer); 

	draw_msgs(); 
}	

void clear_chat_history() {
	curr_num_msgs = curr_msg_row = draw_start_row = 0; 
	draw_msgs(); 
}

void draw_title() {
	
	mvaddstr(0, 0, title1); 
	mvaddstr(1, 0, title2); 
}

void draw_prompt() {
	mvaddstr(LINES - 2, 0, above_prompt); 
	mvaddstr(LINES - 1, 0, prompt); 
	if (command_buffer != NULL && strlen(command_buffer) > 0) {
		mvaddstr(LINES - 1, strlen(prompt) + 1, command_buffer); 
	}
}

void draw_msgs() {

	werase(screen); //Erase screen in case some graphical glitch has occurred like typing weird chars...
	draw_title(); 

	const int CHAT_LINES = LINES - 4; //2 lines for title, 2 lines for prompt, LINES - 4 leftover
	int msg_row = draw_start_row; //Variable indicating what msg we are drawing 
	int num_lines_drawn = 0; //Num lines we've drawn so far in chat window

	/*
	if (curr_num_msgs <= MAX_MSGS) { //Haven't overflowed message history and wrapped around
		msg_row = MAX(0, curr_msg_row - CHAT_LINES); 
	}
	else {//Wrap around message buffer. 
		msg_row = curr_msg_row - CHAT_LINES; 
		if (msg_row < 0)
			msg_row += MAX_MSGS; 
	}
	*/

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

char* curses_input(bool simple_input) {
	int c; //next char input
	//Buffer for storing command user typed
	command_buffer = (char*)malloc(sizeof(char)*(MAX_LINE + 1));
	memset(command_buffer, ' ', MAX_LINE + 1); 
	command_buffer[0] = '\0'; 

	char blanks[MAX_LINE]; 
	memset(blanks, ' ', MAX_LINE); 
	blanks[MAX_LINE - 1] = '\0'; 

	int cmd_i = 0; 
	int x, y; 
	int max_index; 

	if (simple_input) {
		getyx(screen, y, x); 
		wmove(screen, y, strlen(prompt)); 
		int err = getstr(command_buffer); 
		if (err == ERR) {
			add_msg_to_scrn("getstr() failed to return data from stdin"); 
			free(command_buffer); 
			return NULL; 
		}

		mvaddstr(y, strlen(prompt), command_buffer); 
		refresh(); 
		
		return command_buffer; 

	}

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
		else if (c == KEY_PPAGE) {
			scroll_up_one(); 
			draw_msgs(); 
		}
		else if (c == KEY_NPAGE) {
			scroll_down_one(); 
			draw_msgs(); 
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

void scroll_up_one(void) {
	if (curr_num_msgs < MAX_MSGS) {
		draw_start_row = MAX(0, draw_start_row - 1);
	}	
}

void scroll_down_one(void) {
	int CHAT_LINES = LINES - 4; //2 lines for title, 2 lines for prompt, LINES - 4 leftover
	draw_start_row = MAX(0, MIN(curr_msg_row - CHAT_LINES  , draw_start_row + 1)); 
}
