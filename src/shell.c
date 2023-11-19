#include "shell.h"

extern char **environ;

msh_t *alloc_shell(int max_jobs, int max_line, int max_history) {
    msh_t *shell = malloc(sizeof(msh_t));
    // Checks if parameters are 0, if so, set to default constant values
    // Otherwise, set to the parameters provided
    shell->max_jobs = max_jobs == 0 ? 16 : max_jobs; 
    shell->max_line = max_line == 0 ? 1024 : max_line;
    shell->max_history = max_history == 0 ? 10 : max_history;
    // Allocate memory for jobs to the size of max_jobs
    shell->jobs = malloc(shell->max_jobs * sizeof(job_t));
    return shell;
}

int is_empty_or_whitespace(const char *str) {
    // Helper function to check if a string is empty or contains only whitespace
    while (*str != '\0') {
        if (!isspace((unsigned char)*str))
             // Found a non-whitespace character
            return 0; 
        str++;
    }
    // No non-whitespace characters found
    return 1; 
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
    if ((line_ptr == NULL || *line_ptr == '\0') && job_type != NULL) {
        *job_type = 1;
        return NULL;
    }
    // Pointer to the first character of the command
    char * command = line_ptr;
    // Find the first occurence of '&' or ';' in line_ptr and return a pointer to it
    char * job_cat_ptr = strpbrk(line_ptr, "&;");
    // If '&' or ';' is not found, this is the last command in line
    if (job_cat_ptr == NULL && job_type != NULL) {
        *job_type = 1;
        line_ptr = NULL;
        if (is_empty_or_whitespace(command)) {
            // If command is empty or contains only whitespace, return NULL
            return NULL;
        }
        return command;
    } 
    // If '&' or ';' is found, set job_type to 0 or 1 and replace the value of '&' or ';'
    // with '\0' such that command knows where to stop
    if (job_type != NULL) {
        if (*job_cat_ptr == '&') {
            *job_type = 0;
            *job_cat_ptr = '\0';
        } else {
            *job_type = 1;
            *job_cat_ptr = '\0';
        }
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
    if (line == NULL || line[0] == '\0') {
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
    if (argv != NULL) {
        argv[*argc] = NULL;
    }
    return argv;
}

int evaluate(msh_t *shell, char *line) {
    char * command = NULL;
    int job_type = 0;
    int argc;
    pid_t pid;
    char **argv = NULL;
    int max_line_limit = shell->max_line;
    int child_status;
    // Check if the line is too long
    if (strlen(line) > shell->max_line) {
        printf("error: reached the maximum line limit\n");
        return 0;
    }
    do {
        // While there are still commands to parse, parse the command 
        command = parse_tok(command == NULL ? line : NULL, &job_type);
        // Stop parsing if there are no more commands
        if (command != NULL) {
            // For each command, separate the arguments and print them all
            argc = 0;
            argv = separate_args(command, &argc, NULL);
            if (argv == NULL) {
                continue;
            }
            // Check if this is an exit command
            if (strcmp(argv[0], "exit") == 0) {
                free(argv);
                return 1;
            }
            // Check line length
            int line_len = 0;
            for (int i = 0; i < argc; i++) {
                if (line_len > max_line_limit) {
                    free(argv);
                    return 1;
                }
                line_len += strlen(argv[i]);
            }
            // Fork a new child process to handle the execution of the current job
            pid = fork();
            if (pid == 0) {
                // Child executes the command
                if (execve(argv[0], argv, environ) < 0) {
                    printf("%s: Command not found.\n", argv[0]);
                    free(argv);
                    exit(1);
                }
            } else {
                if (job_type == 1) {
                    // Add the foreground job to the jobs array
                    if (! add_job(shell->jobs, shell->max_jobs, pid, FOREGROUND, command)) {
                        // If there is no more capacity for more jobs, print error message and exit
                        free(argv);
                        return 1;
                    }
                } else if (job_type == 0) {
                    // Add the background job to the jobs array
                    if (! add_job(shell->jobs, shell->max_jobs, pid, BACKGROUND, command)) {
                        // If there is no more capacity for more jobs, print error message and exit
                        free(argv);
                        return 1;
                    }
                }
                if (job_type == 1) {
                    // Wait for foreground jobs to finish
                    pid_t w_pid = waitpid(pid, &child_status, WUNTRACED);
                    if (WIFEXITED(child_status)) {
                        delete_job(shell->jobs, shell->max_jobs, w_pid);
                    }
                }                
            }
            free(argv);           
        }
    } while (command != NULL);
        
    for (int i = 0; i < shell->max_jobs; i++) {
        if (shell->jobs[i].state == BACKGROUND) {
            int child_status;
            pid_t w_pid = wait(&child_status);
            if (WIFEXITED(child_status)) {
                delete_job(shell->jobs, shell->max_jobs, shell->jobs[i].pid);
            }
        }
    }

    // Check for any background jobs and free memory
    for (int i = 0; i < shell->max_jobs; i++) {
        if (shell->jobs[i].state == BACKGROUND && shell->jobs[i].cmd_line != NULL) {
            free(shell->jobs[i].cmd_line);
            shell->jobs[i].cmd_line = NULL;
        }
    }
    
    return 0;
}

void exit_shell(msh_t *shell) {
    // To deallocate jobs
    free_jobs(shell->jobs, shell->max_jobs);
    // Deallocate shell memory 
    free(shell);
}