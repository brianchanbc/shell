#ifndef _HISTORY_H_
#define _HISTORY_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char *HISTORY_FILE_PATH;

//Represents the state of the history of the shell
typedef struct history {
    char **lines;
    int max_history;
    int next;
}history_t;

/*
* alloc_history: allocates and initializes the state of the history of the shell
* 
* max_history: The maximum number of saved history commands for the shell
*
* Returns: a history_t pointer that is allocated and initialized
*/
history_t *alloc_history(int max_history);

/*
* add_line_history: add a new line to the history
*
* history: the history state
*
* cmd_line: the command line to be added to the history
*/
void add_line_history(history_t *history, const char *cmd_line);

/*
* print_history: print the history
*
* history: the history state
*/
void print_history(history_t *history);

/*
* find_line_history: find the line at the specified index
* 
* history: the history state
*  
* index: the specified index of the line to be found
*
* Returns: the line at the specified index
*/
char *find_line_history(history_t *history, int index);

/*
* free_history: save all history inside char ** lines array in 
HISTORY_FILE_PATH and free the history state and all allocated memory
* 
* history: the history state
*/
void free_history(history_t *history);

#endif