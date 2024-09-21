#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 100
#define DEFAULT_PATH "/bin"
#define DELIMETER "\t\r\n"

char** parse_input(char* input) {
    char **tokens = malloc(MAX_TOKENS * sizeof(char *));
    char *token;
    int index = 0;

    token = strtok(input, DELIMETER);
    while (token != NULL) {
        tokens[index++] = token;
        token = strtok(NULL, " \t\r\n");
    }
    tokens[index] = NULL; 

    return tokens;
}


void execute_command(char **args, char *path) {
    pid_t pid = fork();  

    if (pid == 0) {  
        
        char exec_path[512];
        snprintf(exec_path, sizeof(exec_path), "%s/%s", path, args[0]);

        execv(exec_path, args);
        if (execv(exec_path, args) == -1) {
            perror("dash: execv failed");
        }
        exit(EXIT_FAILURE);  
    } else if (pid > 0) {  
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("dash: fork failed");
    }
}

int main() {
    char input[MAX_INPUT_SIZE];
    char **args;
    char *path = DEFAULT_PATH;  

    while (1) {
        
        printf("dash> ");
        
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("dash: failed to read input");
            continue;
        }

        args = parse_input(input);

        if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
            break;
        }

        if (args[0] != NULL) {
            execute_command(args, path);
        }

        free(args);
    }

    return 0;
}
