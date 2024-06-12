#define _GNU_SOURCE
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// Function to print the shell prompt
void print_prompt() {
    printf("cssc0000%% ");  // Replace cssc0000 with your class account username
    fflush(stdout);
}

// Function to read the user input
char* read_input() {
    char *input = NULL;
    size_t len = 0;
    getline(&input, &len, stdin);
    input[strcspn(input, "\n")] = 0; // Remove the newline character
    return input;
}

// Function to split input by pipe (|) and execute commands
void execute_command(char *input) {
    char *commands[100];
    char *token = strtok(input, "|");
    int i = 0;

    // Split input by pipe
    while (token != NULL) {
        commands[i++] = token;
        token = strtok(NULL, "|");
    }
    commands[i] = NULL;

    // Handle multiple pipes
    int num_pipes = i - 1;
    int pipefds[2 * num_pipes];

    for (int j = 0; j < num_pipes; j++) {
        if (pipe(pipefds + j * 2) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    int pid;
    int command_index = 0;

    while (commands[command_index] != NULL) {
        pid = fork();
        if (pid == 0) {
            // Redirect stdin for all but the first command
            if (command_index > 0) {
                if (dup2(pipefds[(command_index - 1) * 2], 0) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            // Redirect stdout for all but the last command
            if (commands[command_index + 1] != NULL) {
                if (dup2(pipefds[command_index * 2 + 1], 1) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            // Close all pipe fds
            for (int j = 0; j < 2 * num_pipes; j++) {
                close(pipefds[j]);
            }

            // Parse command arguments
            char *args[100];
            char *arg_token = strtok(commands[command_index], " ");
            int arg_index = 0;

            while (arg_token != NULL) {
                args[arg_index++] = arg_token;
                arg_token = strtok(NULL, " ");
            }
            args[arg_index] = NULL;

            // Execute the command
            execvp(args[0], args);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        command_index++;
    }

    // Close all pipe fds in parent
    for (int j = 0; j < 2 * num_pipes; j++) {
        close(pipefds[j]);
    }

    // Wait for all child processes
    for (int j = 0; j <= num_pipes; j++) {
        wait(NULL);
    }
}
