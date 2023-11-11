#include "shell.h"

msh_t *alloc_shell(int max_jobs, int max_line, int max_history) {
    msh_t *shell = malloc(sizeof(msh_t));
    // Checks if parameters are 0, if so, set to default constant values
    // Otherwise, set to the parameters provided
    shell->max_jobs = max_jobs == 0 ? 16 : max_jobs; 
    shell->max_line = max_line == 0 ? 1024 : max_line;
    shell->max_history = max_history == 0 ? 10 : max_history;
    return shell;
}

char *parse_tok(char *line, int *job_type) {
    // Initialize a static pointer to keep track of index position in line 
    // line_ptr is set to NULL for the first call to parse_tok
    static char *line_ptr = NULL;
    // If line is not NULL, this is the first call, set line_ptr to line
    if (line != NULL) {
        line_ptr = line;
    }
    // If line_ptr is NULL, there is no more commands to parse or it is an empty command
    if (line_ptr == NULL || *line_ptr == '\0') {
        *job_type = -1;
        return NULL;
    }
    // Pointer to the first character of the command
    char * command = line_ptr;
    // Find the first occurence of '&' or ';' in line_ptr and return a pointer to it
    char * job_cat_ptr = strpbrk(line_ptr, "&;");
    // If '&' or ';' is not found, this is the last command in line
    if (job_cat_ptr == NULL) {
        *job_type = 1;
        line_ptr = NULL;
        return command;
    } 
    // If '&' or ';' is found, set job_type to 0 or 1 and replace the value of '&' or ';'
    // with '\0' such that command knows where to stop
    if (*job_cat_ptr == ';') {
        *job_type = 1;
        *job_cat_ptr = '\0';
    } else if (*job_cat_ptr == '&') {
        *job_type = 0;
        *job_cat_ptr = '\0';
    } 
    // Move pointer to the next character after '\0' which we replaced to continue parsing 
    line_ptr = ++job_cat_ptr;
    return command;
}

char **separate_args(char *line, int *argc, bool *is_builtin) {
    char **argv = NULL;
    char *token;
    *argc = 0;
    // If the line is empty, return NULL
    if (line[0] == '\0') {
        return NULL;
    }
    // Tokenize the line and store the tokens in argv
    token = strtok(line, " ");
    while (token != NULL) {
        // Increase the size of argv by 1
        (*argc)++;
        // Resize the memory allocated to argv by 1
        // Size of argv should always be larger than argc by 1 for last NULL element
        argv = realloc(argv, ((*argc)+1) * (sizeof(char *)));
        // Assign token command to argv
        argv[(*argc) - 1] = token;
        // Continue tokenizing the line
        token = strtok(NULL, " ");
    }
    // Add NULL to the end of argv so C knows we have reached the end of the array
    argv[*argc] = NULL;
    return argv;
}

int evaluate(msh_t *shell, char *line) {
    char * command = NULL;
    int job_type = 0;
    int argc;
    char **argv = NULL;
    // Check if the line is too long
    if (strlen(line) > shell->max_line) {
        printf("error: reached the maximum line limit\n");
        return 1;
    }
    do {
        // While there are still commands to parse, parse the command 
        command = parse_tok(command == NULL ? line : NULL, &job_type);
        // Stop parsing if there are no more commands
        if (command != NULL) {
            // For each command, separate the arguments and print them all
            argc = 0;
            argv = separate_args(command, &argc, NULL);
            for (int i = 0; i < argc; i++) {
                printf("argv[%d]=%s\n", i, argv[i]);
            }
            // Free the memory allocated to argv
            free(argv);
            // Also print argument count
            printf("argc=%d\n", argc);
        }
    } while (command != NULL);
    return 0;
}

void exit_shell(msh_t *shell) {
    // Deallocate shell memory 
    free(shell);
}
