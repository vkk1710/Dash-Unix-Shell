#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_TOKENS 100

// Variable to store the search paths for executables (initial value is "/bin")
char *search_paths = NULL;

// Function to initialize the default path (set it to "/bin")
void init_path() {
    search_paths = strdup("/bin");  // Default path is "/bin"
}

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

// Function to find the full path of an executable
char *find_executable(char *command) {
    if (search_paths == NULL) {
        return NULL;  // No valid paths are set
    }

    // Make a copy of search_paths to avoid modifying the original string
    char *paths_copy = strdup(search_paths);
    if (paths_copy == NULL) {
        perror("dash: strdup failed");
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

// Function to execute commands using execv
void execute_command(char **args) {
    if (search_paths == NULL) {
        fprintf(stderr, "dash: no valid paths set, unable to execute external commands\n");
        return;  // No valid paths, only built-in commands can execute
    }

    // Find the full path of the executable
    char *exec_path = find_executable(args[0]);
    if (exec_path == NULL) {
        fprintf(stderr, "dash: command not found: %s\n", args[0]);
        return;
    }

    // Create a new process to execute the command
    pid_t pid = fork();
    if (pid == 0) {  // Child process
        execv(exec_path, args);  // Execute the command
        perror("dash: execv failed");  // If execv fails
        exit(EXIT_FAILURE);
    } else if (pid > 0) {  // Parent process
        int status;
        waitpid(pid, &status, 0);  // Wait for the child process to finish
    } else {
        perror("dash: fork failed");
    }

    free(exec_path);  // Free the allocated exec_path
}

int main(int argc, char *argv[]) {
    char *input = NULL;  // getline will allocate memory for this
    size_t len = 0;  // To store the size of the buffer for getline
    ssize_t nread;  // To store the number of characters read by getline
    char **args;
    FILE *input_stream = stdin;  // Default to standard input (interactive mode)

    // Initialize the path to "/bin"
    init_path();

    // Check if running in batch mode (argc == 2, meaning a file was provided)
    if (argc == 2) {
        // Try to open the batch file
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            perror("dash: cannot open batch file");
            exit(1);  // Exit with an error if the file can't be opened
        }
    } else if (argc > 2) {
        fprintf(stderr, "dash: too many arguments\n");
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

        // Parse the input into arguments
        args = parse_input(input);

        // Check for built-in commands: "cd", "exit", or "path"
        if (args[0] != NULL && strcmp(args[0], "cd") == 0) {
            builtin_cd(args);
        } else if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
            builtin_exit(args);
        } else if (args[0] != NULL && strcmp(args[0], "path") == 0) {
            builtin_path(args);
        } else {
            // If not a built-in command, execute it as an external command
            if (args[0] != NULL) {
                execute_command(args);
            }
        }

        // Free memory for the parsed arguments
        free(args);
    }

    return 0;
}
