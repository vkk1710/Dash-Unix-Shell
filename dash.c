#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_TOKENS 100

// Variable to store the search paths for executables (initial value is "/bin")
char *search_paths = NULL;

// Function to initialize the default path (set it to "/bin")
void init_path() {
    search_paths = strdup("/bin");  // Default path is "/bin"
}

char **parse_input(char *input, char **redirect_file) {
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    char *token;
    int index = 0;
    int redirection_found = 0;

    // Tokenize the input string using space as a delimiter
    token = strtok(input, " \t\r\n");

    while (token != NULL) {
        if (redirection_found) {
            // Error if more tokens appear after redirection
            fprintf(stderr, "An error has occurred\n");
            free(tokens);
            return NULL;
        } else if (strcmp(token, ">") == 0) {
            // If `>` is a separate token, get the next token as the redirection file
            redirection_found = 1;
            token = strtok(NULL, " \t\r\n");
            if (token == NULL) {
                fprintf(stderr, "An error has occurred\n");
                free(tokens);
                return NULL;  // Error: No file specified for redirection
            }
            *redirect_file = token;  // Set the redirection file
        } else if (strchr(token, '>') != NULL) {
            // If `>` is combined with the command (e.g., `test>out`)
            redirection_found = 1;
            char *command_part = strtok(token, ">");  // Split command before `>`
            char *file_part = strtok(NULL, ">");      // Get file name after `>`

            if (command_part != NULL) {
                tokens[index++] = command_part;  // Add command part to tokens
            }
            if (file_part == NULL) {
                fprintf(stderr, "An error has occurred\n");
                free(tokens);
                return NULL;  // Error: No file specified for redirection
            }
            *redirect_file = file_part;  // Set the redirection file
        } else {
            // Otherwise, it's a regular command token
            tokens[index++] = token;
        }

        token = strtok(NULL, " \t\r\n");
    }

    tokens[index] = NULL;  // Null-terminate the list of tokens

    return tokens;
}

// Function to split commands by '&' operator
char **split_commands(char *input) {
    char **commands = malloc(MAX_TOKENS * sizeof(char *));
    char *command;
    int index = 0;
    
    if (input[0] == '&' || input[0] == '>' || input[strlen(input) - 1] == '&' || input[strlen(input) - 1] == '>') {
        fprintf(stderr, "An error has occurred\n");
        free(commands);  // Free commands
        return NULL;
    }
    
    // printf("%c", input[0]);
    command = strtok(input, "&");
    // printf("%s\n", command);

    while (command != NULL) {
        commands[index++] = command;
        command = strtok(NULL, "&");
    }
    commands[index] = NULL;  // Null-terminate the list of commands

    return commands;
}


// Built-in function for 'path'
int builtin_path(char **args) {
    // Free the current search paths
    if (search_paths != NULL) {
        free(search_paths);
    }

    // If no arguments are provided, set search_paths to NULL (disable external commands)
    if (args[1] == NULL) {
        search_paths = NULL;
    } else {
        // Concatenate all the provided paths into one space-separated string
        size_t length = 0;
        int i;
        for (i = 1; args[i] != NULL; i++) {
            length += strlen(args[i]) + 1;  // Add space for each path and a space
        }
        search_paths = malloc(length * sizeof(char));
        search_paths[0] = '\0';  // Initialize as an empty string

        // Copy each argument (path) into the search_paths string
        int j;
        for (j = 1; args[j] != NULL; j++) {
            strcat(search_paths, args[j]);
            if (args[j + 1] != NULL) {
                strcat(search_paths, " ");  // Add a space between paths
            }
        }
    }

    return 1;
}

// Built-in function for 'cd'
int builtin_cd(char **args) {
    if (args[1] == NULL || args[2] != NULL) {
         fprintf(stderr, "An error has occurred\n");
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
        fprintf(stderr, "An error has occurred\n");
        return 1;  // Error, too many arguments
    }
    exit(0);  // Exit with success
}

// Function to find the full path of an executable
char *find_executable(char *command) {
    if (search_paths == NULL) {
        return NULL;  // No valid paths are set
    }

    // Make a copy of search_paths to avoid modifying the original string
    char *paths_copy = strdup(search_paths);
    if (paths_copy == NULL) {
        fprintf(stderr, "An error has occurred\n");
        return NULL;
    }

    // Tokenize the copy of search_paths (space-separated) into individual paths
    char *path = strtok(paths_copy, " ");
    while (path != NULL) {
        char *exec_path = malloc(512 * sizeof(char));
        snprintf(exec_path, 512, "%s/%s", path, command);

        // Check if the file is executable using access()
        if (access(exec_path, X_OK) == 0) {
            free(paths_copy);  // Free the copy of search_paths
            return exec_path;  // Return the full executable path
        }

        free(exec_path);  // Free the current exec_path and move to the next path
        path = strtok(NULL, " ");
    }

    free(paths_copy);  // Free the copy of search_paths
    return NULL;  // No executable found
}

void check_redirection(char *redirect_file, int *saved_stdout, int *saved_stderr) {
    if (redirect_file != NULL) {
            *saved_stdout = dup(STDOUT_FILENO);
            *saved_stderr = dup(STDERR_FILENO);

            // Open the file for writing and truncate if it exists
            int fd = open(redirect_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                fprintf(stderr, "An error has occurred\n");
                exit(EXIT_FAILURE);
            }
            // Redirect stdout and stderr to the file
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);  // Close the file descriptor as it's no longer needed
        }
}

void restore_redirection(int saved_stdout, int saved_stderr) {
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
    if (saved_stderr != -1) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
    }
}


// new funcs -

int is_builtin_command(char *command) {
    return strcmp(command, "cd") == 0 || strcmp(command, "exit") == 0 || strcmp(command, "path") == 0;
}

void handle_builtin_command(char **args) {
    // Check for built-in commands: "cd", "exit", or "path"
    if (strcmp(args[0], "cd") == 0) {

        builtin_cd(args);
    } 
    else if (strcmp(args[0], "exit") == 0) {
        builtin_exit(args);
    } 
    else {
        builtin_path(args);
    }
}

// new execute func
void execute_command(char **commands) {
    int pid, status, saved_stdout, saved_stderr, i, j;
    for (i = 0; commands[i] != NULL; i++) {
        char *redirect_file = NULL;
        char **parsed_args = parse_input(commands[i], &redirect_file);  // Call parse here
        if ((parsed_args != NULL && parsed_args[0] != NULL) && is_builtin_command(parsed_args[0])) {
            handle_builtin_command(parsed_args);
            free(parsed_args);
            continue;  // Exit after handling built-in command
        }
        pid = fork();
        saved_stdout = saved_stderr = -1;

        if (pid == 0) {  
            check_redirection(redirect_file, &saved_stdout, &saved_stderr);

            // Execute the command
            char *exec_path = find_executable(parsed_args[0]);
            if (exec_path == NULL) {
                fprintf(stderr, "An error has occurred\n");
                free(parsed_args);
                exit(EXIT_FAILURE);
            }
            execv(exec_path, parsed_args);  // Execute the command
            fprintf(stderr, "An error has occurred\n");
            free(exec_path);
            restore_redirection(saved_stdout, saved_stderr);
            free(parsed_args);
            exit(EXIT_FAILURE);
        } 
        else if (pid < 0) {
            fprintf(stderr, "An error has occurred\n");
        }
        free(parsed_args);
    }

    // Parent process waits for all child processes to finish
    for (j = 0; commands[j] != NULL; j++) {
        wait(&status);
    }

    free(commands);  // Free the command list
}



int main(int argc, char *argv[]) {
    char *input = NULL;  // getline will allocate memory for this
    size_t len = 0;  // To store the size of the buffer for getline
    ssize_t nread;  // To store the number of characters read by getline
    char **commands;
    FILE *input_stream = stdin;  // Default to standard input (interactive mode)

    // Initialize the path to "/bin"
    init_path();

    // Check if running in batch mode (argc == 2, meaning a file was provided)
    if (argc == 2) {
        // Try to open the batch file
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            fprintf(stderr, "An error has occurred\n");
            exit(1);  // Exit with an error if the file can't be opened
        }
    } else if (argc > 2) {
        fprintf(stderr, "An error has occurred\n");
        exit(1);
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

        commands = split_commands(input);

        if (commands != NULL){
            execute_command(commands);
        } 
        
    }

    return 0;
}
