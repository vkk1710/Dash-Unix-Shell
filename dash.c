#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_TOKENS 550

// Variable to store the shell path for executables (initial value is "/bin")
char *search_paths = NULL;

// Function to initialize the default path (set it to "/bin")
void init_path() {
    search_paths = strdup("/bin");  // Default path is "/bin"
}

// Function to parse input string, tokenize by using space, tab, etc as delimiter and identify redirect file
char **parse_command(char *input, char **redirect_file) {
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    char *token;
    int index = 0;
    int redirection_found = 0;

    // Tokenize the input string using space, tab, etc as delimiters
    token = strtok(input, " \t\r\n");

    while (token != NULL) {
        if (redirection_found) {
            // Error if more tokens appear after redirection
            fprintf(stderr, "An error has occurred\n");
            free(tokens);
            return NULL;

        } 
        else if (strcmp(token, ">") == 0) {
            // Case - If `>` is a separate token, get the next token as the redirection file

            // Set the redirection_found flag as 1.
            redirection_found = 1;

            // Go to the next token.
            token = strtok(NULL, " \t\r\n");
            
            // Check if a file name is specified after the redirection symbol or not. If not raise error.
            if (token == NULL) {
                fprintf(stderr, "An error has occurred\n");
                free(tokens);
                return NULL;
            }

            // Set the redirection file
            *redirect_file = token;

        } 
        else if (strchr(token, '>') != NULL) {
            // Case - If `>` is combined with the command (e.g., `test>out`)

            // Set the redirection_found flag as 1.
            redirection_found = 1;

            char *command_part = strtok(token, ">");  // Split command before `>`
            char *file_part = strtok(NULL, ">");      // Get file name after `>`

            // Add command part to tokens if not NULL
            if (command_part != NULL) {
                tokens[index++] = command_part;
            }

            if (file_part == NULL) {
                fprintf(stderr, "An error has occurred\n");
                free(tokens);
                return NULL;
            }

            // Set the redirection file
            *redirect_file = file_part;

        } 
        else {
            // Case - It is a regular command token
            tokens[index++] = token;
        }

        token = strtok(NULL, " \t\r\n");
    }

    // Null-terminate the list of tokens
    tokens[index] = NULL;

    return tokens;
}

// Function to split input stirng into commands by using '&' operator as the delimiter
char **split_commands(char *input) {
    char **commands = malloc(MAX_TOKENS * sizeof(char *));
    char *command;
    int index = 0;
    
    if (input[0] == '&' || input[0] == '>' || input[strlen(input) - 1] == '&' || input[strlen(input) - 1] == '>') {
        fprintf(stderr, "An error has occurred\n");
        free(commands);
        return NULL;
    }
    
    // Tokenize the input string using & as the delimiter
    command = strtok(input, "&");

    while (command != NULL) {
        commands[index++] = command;
        command = strtok(NULL, "&");
    }

    // Null-terminate the list of commands
    commands[index] = NULL;

    return commands;
}


// Function for built-in 'path' command
int builtin_path(char **args) {

    // Free the current search paths
    if (search_paths != NULL) {
        free(search_paths);
    }

    // If no arguments are provided, set search_paths to NULL (disable external commands)
    if (args[1] == NULL) {
        search_paths = NULL;
    } 
    else {
        // Concatenate all the provided paths into one space-separated string
        size_t length = 0;
        int i;
        for (i = 1; args[i] != NULL; i++) {
            // Count the space after each directory in path
            length += strlen(args[i]) + 1;
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

// Function for built-in 'cd' command
int builtin_cd(char **args) {

    // Syntax error.
    if (args[1] == NULL || args[2] != NULL) {
         fprintf(stderr, "An error has occurred\n");
        return 1;  // Error, too many arguments
    }
    // Change directory using chdir()
    if (chdir(args[1]) != 0) {
        fprintf(stderr, "An error has occurred\n");
    }

    return 1;
}

// Function for built-in 'exit' command
int builtin_exit(char **args) {
    
    // Error, too many arguments
    if (args[1] != NULL) {
        fprintf(stderr, "An error has occurred\n");
        return 1;  // Error, too many arguments
    }
    exit(0);  // Exit with success
}

char *find_executable(char *command) {
    if (search_paths == NULL) {
        return NULL;  // No valid paths are set
    }

    // Make a copy of search_paths to avoid modifying the original string
    char *paths_copy = strdup(search_paths);
    if (paths_copy == NULL) {  // Check if strdup() failed
        fprintf(stderr, "An error has occurred\n");
        return NULL;  // Return NULL if memory allocation failed
    }

    // Tokenize the copy of search_paths (space-separated) into individual paths
    char *path = strtok(paths_copy, " ");
    while (path != NULL) {
        // Dynamically allocate enough space for the full path + command + '/' + '\0'
        char *exec_path = malloc(strlen(path) + strlen(command) + 2);  // +2 for '/' and '\0'
        if (exec_path == NULL) {
            free(paths_copy);  // Free paths_copy before returning
            fprintf(stderr, "An error has occurred\n");
            return NULL;  // Return NULL if memory allocation failed
        }

        // Build the full path: path/command
        snprintf(exec_path, strlen(path) + strlen(command) + 2, "%s/%s", path, command);

        // Check if the file is executable using access()
        if (access(exec_path, X_OK) == 0) {
             free(paths_copy);  // Free paths_copy before returning the exec_path
            return exec_path;  // Return the full executable path
        }

        free(exec_path);  // Free the current exec_path and move to the next path
        path = strtok(NULL, " ");
    }
<<<<<<< HEAD

    free(paths_copy);  // Free paths_copy when no executable is found
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

void execute_command(char **commands) {
    int pid, status, saved_stdout, saved_stderr, i, j;

    for (i = 0; commands[i] != NULL; i++) {
        char *redirect_file = NULL;

        // Parse each command by parse_command function
        char **parsed_args = parse_command(commands[i], &redirect_file);

        // Check if it is a built-in command.
        if ((parsed_args != NULL && parsed_args[0] != NULL) && is_builtin_command(parsed_args[0])) {
            handle_builtin_command(parsed_args);
            free(parsed_args);
            continue;                               // Exit after handling built-in command
        }

        // Fork a child process.
        pid = fork();
        saved_stdout = saved_stderr = -1;

        if (pid == 0) {  
            // Child process.

            // Check redirection by calling check_redirection fucntion
            check_redirection(redirect_file, &saved_stdout, &saved_stderr);

            // Find path for executable of the command.
            char *exec_path = find_executable(parsed_args[0]);
            if (exec_path == NULL) {
                fprintf(stderr, "An error has occurred\n");
                free(parsed_args);
                exit(EXIT_FAILURE);
            }

            // Use execv to execute command.
            execv(exec_path, parsed_args);

            // Print error if execv fails and free exec_path.
            fprintf(stderr, "An error has occurred\n");
            // free(exec_path);
            restore_redirection(saved_stdout, saved_stderr);
            free(parsed_args);

            // Exit failure.
            exit(EXIT_FAILURE);
        } 
        else if (pid < 0) {
            // Error in creating a child process.
            fprintf(stderr, "An error has occurred\n");
        }
        //free(parsed_args);
    }

    // Parent process waits for all child processes to finish.
    for (j = 0; commands[j] != NULL; j++) {
        wait(&status);
    }

    // Free commands.
    free(commands);
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

        // Open the batch file
        input_stream = fopen(argv[1], "r");

        // Exit with an error if the file can't be opened
        if (input_stream == NULL) {
            fprintf(stderr, "An error has occurred\n");
            exit(1);
        }

    } 

    // If more arguments are given, print error and exit.
    else if (argc > 2) {
        fprintf(stderr, "An error has occurred\n");
        exit(1);
    }

    // Run shell.
    while (1) {
        // If in interactive mode, print the prompt
        if (input_stream == stdin) {
            printf("dash> ");
        }

        // Read the next command from input_stream (stdin or batch file) using getline()
        nread = getline(&input, &len, input_stream);

        // Case - End-of-file or error
        if (nread == -1) {  
            // Free the input buffer allocated by getline, close the file in batch mode and exit.

            free(input);  
            if (input_stream != stdin) {
                fclose(input_stream);
            }
            exit(0);
        }

        // Split commands.
        commands = split_commands(input);

        // Execute commands.
        if (commands != NULL){
            execute_command(commands);
        } 
        
    }

    return 0;
}
