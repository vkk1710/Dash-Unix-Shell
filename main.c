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

        // If the user types "exit", exit the shell
        if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
            free(args);
            free(input);  // Free the input buffer allocated by getline
            if (input_stream != stdin) {
                fclose(input_stream);  // Close the file in batch mode
            }
            exit(0);  // Exit gracefully
        }

        // If the input is not empty, execute the command
        if (args[0] != NULL) {
            execute_command(args, path);
        }

        // Free memory for the parsed arguments
        free(args);
    }

    return 0;
}