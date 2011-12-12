#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX(x,y) ((x) < (y) ? (y) : (x))
#define MIN(x,y) ((x) > (y) ? (y) : (x))

//Removes whitespace from end of string
void trim_str(char* str); 

//Writes formatted string with current time / date to str
void get_current_time(char* str); 
