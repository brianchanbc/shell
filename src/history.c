#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *HISTORY_FILE_PATH = "../data/.msh_history";

history_t *alloc_history(int max_history) {
    // Allocate memory for history
    history_t *history = malloc(sizeof(history_t));
    history->lines = malloc(max_history * sizeof(char *));
    history->max_history = max_history;
    history->next = 0;

    // Read in history commands from file and insert into lines
    FILE *fp = fopen(HISTORY_FILE_PATH, "r");
    if (fp == NULL) {
        // File does not exist, return history
        return history; 
    }
    // File exists, read in history commands
    char *line = NULL;
    long int len = 0;
    long nread = getline(&line, &len, fp);
    while (nread != -1 && history->next < history->max_history) {
        // Add line to history
        add_line_history(history, line);
        // Free line
        free(line);
        line = NULL;
        // Read next line
        nread = getline(&line, &len, fp);
    }
    // Close file
    fclose(fp);
    return history;
}

void add_line_history(history_t *history, const char *cmd_line) {
    if (cmd_line == NULL) {
        return;
    }
    // If history is full, remove the first line
    // Then shift all lines up by 1
    if (history->next == history->max_history) {
        free(history->lines[0]);
        for (int i = 0; i < history->next - 1; i++) {
            history->lines[i] = history->lines[i+1];
        }
        history->next--;
    }
    // Copy command line up to newline character (i.e. not copy newline character)
    int len = strcspn(cmd_line, "\n");
    // strdup allocates memory for the string copied over, to be freed later
    history->lines[history->next] = strndup(cmd_line, len);
    history->next++;
}

void print_history(history_t *history) {
    for(int i = 1; i <= history->next; i++) {
        printf("%5d\t%s\n",i,history->lines[i-1]);
    }
}

char *find_line_history(history_t *history, int index) {
    if (index < 1 || index > history->next) {
        // Return NULL if index is out of bounds
        return NULL;
    }
    return history->lines[index-1];
}

void free_history(history_t *history) {
    FILE * fp = fopen(HISTORY_FILE_PATH, "w");
    // Check if file opened successfully
    if (fp == NULL) {
        char *updated_path = "./data/.msh_history";
        fp = fopen(updated_path, "w");
        if (fp == NULL) {
            perror("Error opening history file");
            exit(EXIT_FAILURE);
        }
    }
    // Write history to file
    for (int i = 0; i < history->next; i++) {
        // Write line to file
        fprintf(fp, "%s", history->lines[i]);
        if (i < history->next - 1) {
            // Add new line character if not last line
            fputc('\n', fp);
        }
        // Free respective line
        free(history->lines[i]);
    }
    // Close file
    fclose(fp);
    // Free remaining memory
    free(history->lines);
    free(history);
}
