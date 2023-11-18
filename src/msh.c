#define _GNU_SOURCE
#include "shell.h"

int parse_option(char opt, char* optarg, int* option);
int optional_args(int* argc, char* argv[], int* s, int* j, int* l);

int main(int argc, char *argv[]) {
    /*
    The main function performs a few tasks:
    1. Parse the optional arguments
    2. Initialize the shell and allocate memory
    3. Read input from stdin until the user exits or a file
    4. Evaluate the input
    5. Free the shell memory

    Arguments:
    argc: The number of arguments passed to the shell
    argv: The arguments passed to the shell. 
    The first argument is the name of the shell executable.
    The remaining arguments are optional arguments that are parsed by the shell.
    */
    
    // Parse optional arguments
    int s = 0, j = 0, l = 0, op_status = 0;
    op_status = optional_args(&argc, argv, &s, &j, &l);
    if (op_status == 1) {
        // If optional arguments are not valid, print usage requirements and exit
        printf("usage: msh [-s NUMBER] [-j NUMBER] [-l NUMBER]\n"); 
        return 1;
    }

    // Initialize the shell and allocate memory
    msh_t *shell = alloc_shell(j, l, s);

    char *line = NULL;
    size_t len = 0;
    printf("msh> ");
    // Read the first line
    long nRead = getline(&line, &len, stdin);
    while ( nRead != -1) {
        // Check if the command is exit, if so then exit the shell
        if (strcmp(line, "exit\n") == 0) {
            free(line);
            break;
        }
        // Evaluate the command
        if (strlen(line) > 0) {
            line[strlen(line) - 1] = '\0';
        }
        evaluate(shell, line);
        // Free line memory before getting the next line
        free(line);
        // Reset line back to null for the next line
        line = NULL;
        printf("msh> ");
        nRead = getline(&line, &len, stdin);
    }
    
    // Free the shell memory
    exit_shell(shell);
    return 0;
}

int parse_option(char opt, char* optarg, int* option) {
    /*
    Helper function to parse optional arguments

    Arguments:
    opt: The optional argument
    optarg: The optional argument value
    option: The type of optional argument to parse
    */

    int op_val;
    // Check if the optional argument is a valid positive integer
    if (sscanf(optarg, "%d", &op_val) != 1 || op_val <= 0) {
        return 1;
    }
    // If input is valid, assign the optional argument value to the pointer of the option
    *option = op_val;
    return 0;
}

int optional_args(int* argc, char* argv[], int* s, int* j, int* l) {
    /*
    Function to parse optional arguments

    Arguments:
    argc: The number of arguments passed to the shell
    argv: The arguments passed to the shell.
    s: The maximum number of command lines to store in the shell history
    j: The maximum number of jobs that can be in existence at any point in time
    l: The maximum number of characters that can be entered on a single command line
    s, j and l are to be updated if the respective optional arguments are parsed
    */

    int opt = 0;
    opterr = 0;

    int is_integer(const char *str) {
        char *end;
        strtol(str, &end, 10);
        return end != str && *end == '\0';
    }
    for (int i = 1; i < *argc; i++) {
        // Check if optional argument other than -l, -s, -j or their respective numbers are provided
        if (strcmp(argv[i], "-l") != 0 && strcmp(argv[i], "-s") != 0 && strcmp(argv[i], "-j") != 0 && !is_integer(argv[i])) {
            return 1;
        }
    }

    // Parse optional arguments
    while((opt = getopt(*argc, argv, "j:l:s:")) != -1)  
    {  
        // Check if optional argument is provided but value is not provided
        if (optarg == NULL || optarg[0] == '-') {
            return 1;
        }
        switch(opt)  
        {  
            case 'j': 
                if (parse_option(opt, optarg, j)) {
                    return 1;
                }
                break;
            case 'l': 
                if (parse_option(opt, optarg, l)) {
                    return 1;
                }
                break;
            case 's': 
                if (parse_option(opt, optarg, s)) {
                    return 1;
                }
                break;
            case ':': // if optional command argument value is not provided
            case '?': // if optional command is not recognizable
                return 1;
        }  
    }
    return 0;
}