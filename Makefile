TO_COMPILE = curses_client.c modules/*.c
ALL_FILES = curses_client.c include/*.h modules/*.c
FLAGS = -g -Wall -pthread
OUTPUT = run_client
LIBS = -lcurses

test: $(ALL_FILES)
	gcc -o $(OUTPUT) $(FLAGS) $(TO_COMPILE) $(LIBS)

