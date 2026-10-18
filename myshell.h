#ifndef MYSHELL_H
#define MYSHELL_H

#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_COMMANDS 8

typedef struct {
    char *argv[MAX_ARGS];
    char *input_file;
    char *output_file;
    int append_output;
    char *_owned; /* backing buffer for argv strings, freed by free_parsed_line */
} Command;

typedef struct {
    Command commands[MAX_COMMANDS];
    int command_count;
    int background;
} ParsedLine;

/* parser.c */
int parse_line(char *line, ParsedLine *parsed);
void free_parsed_line(ParsedLine *parsed);

/* executor.c */
int execute_builtin(Command *cmd, int *handled);
int execute_pipeline(ParsedLine *parsed);
int handle_redirection(Command *cmd);
void cleanup_background_processes(void);
void register_background_pid(pid_t pid);

/* myshell.c */
void print_prompt(void);
char *read_input(void);

#endif
