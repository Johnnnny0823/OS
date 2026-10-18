#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include "myshell.h"

#define MAX_BG_PIDS 256

static pid_t bg_pids[MAX_BG_PIDS];
static int bg_count = 0;

void register_background_pid(pid_t pid) {
    if (bg_count < MAX_BG_PIDS) {
        bg_pids[bg_count++] = pid;
    }
    printf("[background pid: %d]\n", pid);
}

void cleanup_background_processes(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < bg_count; i++) {
            if (bg_pids[i] == pid) {
                bg_pids[i] = bg_pids[bg_count - 1];
                bg_count--;
                break;
            }
        }
    }
}

/* returns 0 if not a builtin, 1 if builtin and handled. *handled set accordingly */
int execute_builtin(Command *cmd, int *handled) {
    *handled = 0;
    if (cmd->argv[0] == NULL) return 0;

    if (strcmp(cmd->argv[0], "exit") == 0) {
        *handled = 1;
        exit(0);
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        *handled = 1;
        const char *target = cmd->argv[1];
        if (target == NULL) {
            target = getenv("HOME");
            if (target == NULL) target = ".";
        }
        if (chdir(target) != 0) {
            fprintf(stderr, "myOSshell: cd: no such file or directory\n");
        }
        return 1;
    }

    if (strcmp(cmd->argv[0], "pwd") == 0) {
        *handled = 1;
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("myOSshell: pwd");
        }
        return 1;
    }

    return 0;
}

/* apply redirection for a single command in the current process (used pre-exec in child) */
int handle_redirection(Command *cmd) {
    if (cmd->input_file != NULL) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "myOSshell: %s: no such file or directory\n", cmd->input_file);
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("myOSshell: dup2");
            close(fd);
            return -1;
        }
        close(fd);
    }

    if (cmd->output_file != NULL) {
        int flags = O_WRONLY | O_CREAT | (cmd->append_output ? O_APPEND : O_TRUNC);
        int fd = open(cmd->output_file, flags, 0644);
        if (fd < 0) {
            perror("myOSshell: open failed");
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("myOSshell: dup2");
            close(fd);
            return -1;
        }
        close(fd);
    }

    return 0;
}

static void run_child(Command *cmd) {
    if (handle_redirection(cmd) != 0) {
        exit(1);
    }
    execvp(cmd->argv[0], cmd->argv);
    /* execvp only returns on failure */
    fprintf(stderr, "myOSshell: command not found: %s\n", cmd->argv[0]);
    exit(127);
}

int execute_pipeline(ParsedLine *parsed) {
    int n = parsed->command_count;
    if (n == 0) return 0;

    /* single command, no pipe: handle builtins (cd/pwd/exit) in the shell process itself,
       since they must affect the shell's own state rather than a child's */
    if (n == 1 && !parsed->background) {
        int handled = 0;
        execute_builtin(&parsed->commands[0], &handled);
        if (handled) return 0;
    }

    int pipefds[2 * (MAX_COMMANDS - 1)];
    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("myOSshell: pipe failed");
            return -1;
        }
    }

    pid_t pids[MAX_COMMANDS];

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("myOSshell: fork failed");
            return -1;
        }

        if (pid == 0) {
            /* child */
            if (i > 0) {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }
            if (i < n - 1) {
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }
            for (int j = 0; j < 2 * (n - 1); j++) {
                close(pipefds[j]);
            }
            run_child(&parsed->commands[i]);
            /* unreachable */
        } else {
            pids[i] = pid;
        }
    }

    for (int j = 0; j < 2 * (n - 1); j++) {
        close(pipefds[j]);
    }

    if (parsed->background) {
        for (int i = 0; i < n; i++) {
            register_background_pid(pids[i]);
        }
        return 0;
    }

    int status;
    for (int i = 0; i < n; i++) {
        waitpid(pids[i], &status, 0);
    }
    return 0;
}
