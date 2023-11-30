#include "shell.h"

extern char **environ;
extern msh_t *shell;
extern volatile sig_atomic_t fg_pid;

msh_t *alloc_shell(int max_jobs, int max_line, int max_history) {
    msh_t *shell = malloc(sizeof(msh_t));
    // Checks if parameters are 0, if so, set to default constant values
    // Otherwise, set to the parameters provided
    shell->max_jobs = max_jobs == 0 ? 16 : max_jobs; 
    shell->max_line = max_line == 0 ? 1024 : max_line;
    shell->max_history = max_history == 0 ? 10 : max_history;
    // Allocate memory for jobs to the size of max_jobs
    shell->jobs = malloc(shell->max_jobs * sizeof(job_t));
    // Allocate memory for history to the size of max_history
    shell->history = alloc_history(shell->max_history);
    // Initialize jobs
    initialize_signal_handlers();
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

    // Check if line is a standalone "exit"
    bool is_standalone_exit = strcmp(line, "exit") == 0;
    // Check if line is empty
    bool is_invalid_line = strcmp(line, "") == 0 || strcmp(line, "\n") == 0;
    // Check if line is a history command
    bool is_history_command = line[0] == '!';
    if (!is_standalone_exit && !is_invalid_line && !is_history_command) {
        // Add line to history
        add_line_history(shell->history, line);
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
            // Check and if applicable, execute built-in commands
            char *builtin_command = builtin_cmd(argv);
            if (builtin_command != NULL && builtin_command != "1") {
                // Execute the built-in command from history
                int status = evaluate(shell, builtin_command);
            } else if (builtin_command == "1") {
                // Not a built-in command, fork a new child process to execute the command
                // Initialize for signal handling
                sigset_t mask_one, prev_one, mask_all, prev_all;
                Sigfillset(&mask_all);
                Sigemptyset(&mask_one);
                Sigaddset(&mask_one, SIGCHLD);
                // Block child process
                Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
                // Fork a new child process to handle the execution of the current job
                pid = fork();
                if (pid == 0) {
                    // Unblock child process
                    Sigprocmask(SIG_SETMASK, &prev_one, NULL);
                    // Put the child in a new process group whose group ID is identical to the child’s PID
                    Setpgid(0, 0);
                    // Child executes the command
                    if (execve(argv[0], argv, environ) < 0) {    
                        printf("%s: Command not found.\n", argv[0]);
                        free(argv);
                        exit(1);
                    }
                } else {
                    // Block parent process
                    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
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
                    // Unblock parent process
                    Sigprocmask(SIG_SETMASK, &prev_all, NULL);
                    
                    // If the job is a foreground job, wait for it to finish
                    if (job_type == 1) {
                        fg_pid = pid;
                        // When the foreground job terminates, fg_pid will be set to 0
                        // Before that, parent wait for child to terminate
                        while (fg_pid != 0) {
                            Sigsuspend(&prev_one);
                        }
                    } else {
                        // If the job is a background job, print the job info
                        printf("pid %d %s \t %s\n", pid, "Running", command);
                    }   
                }
                // Unblock child process
                Sigprocmask(SIG_SETMASK, &prev_one, NULL);
                // Deallocate argv
                free(argv);  
            }         
        }
    } while (command != NULL);
    
    return 0;
}

char *builtin_cmd(char **argv) {
    // Check if the command is a built-in command
    if (strcmp(argv[0], "jobs") == 0) {
        // If the command is jobs, print the jobs
        print_jobs(shell->jobs, shell->max_jobs);
        return NULL;
    } else if (strcmp(argv[0], "history") == 0) {
        // If the command is history, print the history
        print_history(shell->history);
        return NULL;
    } else if (argv[0][0] == '!') {
        // If the command is a specific history command, find the command in history
        int history_num = atoi(&argv[0][1]);
        char *command = find_line_history(shell->history, history_num);
        if (command == NULL) {
            printf("!%d: No such command in history\n", history_num);
            return NULL;
        }
        printf("%s\n", command);
        // Return the command, which will be executed in evaluate
        return command;
    } else if (strcmp(argv[0], "bg") == 0) {
        // If the command is bg, bring the job to the background
        if (argv[1] == NULL) {
            // If no job number is provided, print error message
            printf("bg: No job number provided\n");
            return NULL;
        }
        // Check if the job number is a JOB_ID or PID
        char* job_arg = argv[1];
        bool is_job_id = job_arg[0] == '%';
        int job_num = atoi(is_job_id ? job_arg + 1 : job_arg);
        pid_t pid;
        int job_id;
        if (is_job_id) {
            // If it's a JOB_ID, find the corresponding job and get its 
            job_id = job_num;
            pid = get_job_pid(shell->jobs, shell->max_jobs, job_num);
        } else {
            // If it's a PID, find the corresponding job and get its JOB_ID
            job_id = get_job_jid(shell->jobs, shell->max_jobs, job_num);
            pid = job_num;
        }
        if (pid == -1 || job_id == 0) {
            // If the job number is not valid, print error message
            printf("bg: Invalid job number\n");
            return NULL;
        }
        // Resume the job
        change_job_state(shell->jobs, shell->max_jobs, job_id, BACKGROUND);
        Kill(-pid, SIGCONT);
        return NULL;
    } else if (strcmp(argv[0], "fg") == 0) {
        // If the command is fg, bring the job to the foreground
        if (argv[1] == NULL) {
            // If no job number is provided, print error message
            printf("fg: No job number provided\n");
            return NULL;
        }
        // Check if the job number is a JOB_ID or PID
        char* job_arg = argv[1];
        bool is_job_id = job_arg[0] == '%';
        int job_num = atoi(is_job_id ? job_arg + 1 : job_arg);
        pid_t pid;
        int job_id;
        if (is_job_id) {
            // If it's a JOB_ID, find the corresponding job and get its 
            job_id = job_num;
            pid = get_job_pid(shell->jobs, shell->max_jobs, job_num);
        } else {
            // If it's a PID, find the corresponding job and get its JOB_ID
            job_id = get_job_jid(shell->jobs, shell->max_jobs, job_num);
            pid = job_num;
        }
        if (pid == -1 || job_id == 0) {
            // If the job number is not valid, print error message
            printf("fg: Invalid job number\n");
            return NULL;
        }
        // Resume the job
        change_job_state(shell->jobs, shell->max_jobs, job_id, FOREGROUND);
        Kill(-pid, SIGCONT);
        return NULL;
    } else if (strcmp(argv[0], "kill") == 0) {
        if (argv[1] == NULL || argv[2] == NULL) {
            printf("kill: Not enough arguments\n");
            return NULL;
        }
        int pid = atoi(argv[2]);
        // Send kill signal
        if (strcmp(argv[1], "2") == 0) {
            kill(-pid, SIGINT);
        } else if (strcmp(argv[1], "9") == 0) {
            kill(-pid, SIGKILL);
        } else if (strcmp(argv[1], "18") == 0) {
            kill(-pid, SIGCONT);
        } else if (strcmp(argv[1], "19") == 0) {
            kill(-pid, SIGSTOP);
        } else {
            printf("error: invalid signal number\n");
            return NULL;
        }
        return NULL;
    }
    // If the command is not a built-in command, return "1"
    char * not_builtin = "1";
    return not_builtin;
}

void exit_shell(msh_t *shell) {
    // Deallocate history
    free_history(shell->history);
    // Deallocate jobs
    free_jobs(shell->jobs, shell->max_jobs);
    // Deallocate shell memory
    free(shell);
}