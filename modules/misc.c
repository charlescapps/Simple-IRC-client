#include "../include/misc.h"

void trim_str(char* str) {
	int len = strlen(str); 
	int i = len - 1; 

	while (i >=0 && (str[i] == '\n' || str[i] == ' ' || str[i] == '\t')) {
		--i; 
	}

	str[i+1] = '\0'; 
}

void get_current_time(char* str) {

	time_t raw_time; 
	time(&raw_time); 
	struct tm* time_info = localtime(&raw_time);  
	strftime (str, 1024, "%c", time_info ); 
	//free(time_info); 

}
