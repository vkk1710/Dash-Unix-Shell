#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TOKENS 100
#define DEFAULT_PATH "/bin"

// Function to parse the input
char **parse_input(char *input) {
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    char *token;
    int index = 0;

    // Tokenize the input string using space as a delimiter
    token = strtok(input, " \t\r\n");
    while (token != NULL) {
        tokens[index++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    tokens[index] = NULL;  // Null-terminate the list of tokens

    return tokens;
}

// Function to execute commands using execv
void execute_command(char **args, char *path) {
    pid_t pid = fork();  // Create a new process

    if (pid == 0) {  // Child process
        // Construct the full path to the executable
        char exec_path[512];
        snprintf(exec_path, sizeof(exec_path), "%s/%s", path, args[0]);

        // Execute the command using execv
        if (execv(exec_path, args) == -1) {
            perror("dash: execv failed");
        }
        exit(EXIT_FAILURE);  // If execv fails, exit child process
    } else if (pid > 0) {  // Parent process
        // Wait for the child process to finish
        int status;
        waitpid(pid, &status, 0);
    } else {
        // Fork failed
        perror("dash: fork failed");
    }
}

// Built-in function for 'cd'
int builtin_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "dash: cd: missing argument\n");
        return 1;  // Error, missing argument
    } else if (args[2] != NULL) {
        fprintf(stderr, "dash: cd: too many arguments\n");
        return 1;  // Error, too many arguments
    }

    // Change directory using chdir()
    if (chdir(args[1]) != 0) {
        perror("dash: cd");
    }

    return 1;
}

// Built-in function for 'exit'
int builtin_exit(char **args) {
    if (args[1] != NULL) {
        fprintf(stderr, "dash: exit: too many arguments\n");
        return 1;  // Error, too many arguments
    }
    exit(0);  // Exit with success
}

int main(int argc, char *argv[]) {
    char *input = NULL;  // getline will allocate memory for this
    size_t len = 0;  // To store the size of the buffer for getline
    ssize_t nread;  // To store the number of characters read by getline
    char **args;
    char *path = DEFAULT_PATH;  // Initial path set to /bin
    FILE *input_stream = stdin;  // Default to standard input (interactive mode)

    // Check if running in batch mode (argc == 2, meaning a file was provided)
    if (argc == 2) {
        // Try to open the batch file
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            perror("dash: cannot open batch file");
            exit(EXIT_FAILURE);  // Exit with an error if the file can't be opened
        }
    } else if (argc > 2) {
        fprintf(stderr, "dash: too many arguments\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // If in interactive mode, print the prompt
        if (input_stream == stdin) {
            printf("dash> ");
        }

        // Read the next command from input_stream (stdin or batch file) using getline()
        nread = getline(&input, &len, input_stream);
        if (nread == -1) {  // End-of-file or error
            free(input);  // Free the input buffer allocated by getline
            if (input_stream != stdin) {
                fclose(input_stream);  // Close the file in batch mode
            }
            exit(0);  // Exit gracefully
        }

        // Parse the input into arguments
        args = parse_input(input);

        // Check if the command is "cd" and handle it as a built-in command
        if (args[0] != NULL && strcmp(args[0], "cd") == 0) {
            builtin_cd(args);
        }
        // Check if the command is "exit" and handle it as a built-in command
        else if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
            builtin_exit(args);
        }
        // If not a built-in command, execute it as an external command
        else {
            if (args[0] != NULL) {
                execute_command(args, path);
            }
        }

        // Free memory for the parsed arguments
        free(args);
    }

    return 0;
}
