#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "myshell.h"

void print_prompt(void) {
    printf("myOSshell $ ");
    fflush(stdout);
}

char *read_input(void) {
    static char line[MAX_LINE];
    if (fgets(line, sizeof(line), stdin) == NULL) {
        return NULL; /* EOF, e.g. Ctrl+D */
    }
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
    return line;
}

static void sigchld_handler(int sig) {
    (void)sig;
    cleanup_background_processes();
}

int main(void) {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        print_prompt();

        char *line = read_input();
        if (line == NULL) {
            printf("\n");
            break; /* EOF */
        }

        if (strlen(line) == 0) {
            continue; /* blank input, just reprompt */
        }

        ParsedLine parsed;
        int rc = parse_line(line, &parsed);
        if (rc != 0) {
            continue; /* parse error already reported */
        }
        if (parsed.command_count == 0) {
            continue;
        }

        execute_pipeline(&parsed);
        free_parsed_line(&parsed);

        cleanup_background_processes();
    }

    return 0;
}
