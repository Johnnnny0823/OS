#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "myshell.h"

/* trim leading/trailing whitespace in place */
static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return s;
}

/* split a single command segment (no pipes) into argv[], input_file, output_file, append_output */
static int parse_segment(char *segment, Command *cmd) {
    cmd->argv[0] = NULL;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_output = 0;

    int argc = 0;
    char *p = segment;

    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        if (*p == '<') {
            p++;
            while (*p == ' ' || *p == '\t') p++;
            char *start = p;
            while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '<' && *p != '>') p++;
            if (start == p) {
                fprintf(stderr, "myOSshell: missing input file\n");
                return -1;
            }
            char saved = *p;
            *p = '\0';
            cmd->input_file = start;
            if (saved != '\0') p++;
            continue;
        }

        if (*p == '>') {
            p++;
            int append = 0;
            if (*p == '>') {
                append = 1;
                p++;
            }
            while (*p == ' ' || *p == '\t') p++;
            char *start = p;
            while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '<' && *p != '>') p++;
            if (start == p) {
                fprintf(stderr, "myOSshell: missing output file\n");
                return -1;
            }
            char saved = *p;
            *p = '\0';
            cmd->output_file = start;
            cmd->append_output = append;
            if (saved != '\0') p++;
            continue;
        }

        char *start = p;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '<' && *p != '>') p++;
        char saved = *p;
        *p = '\0';
        if (argc < MAX_ARGS - 1) {
            cmd->argv[argc++] = start;
        }
        if (saved != '\0') p++;
    }

    cmd->argv[argc] = NULL;

    if (argc == 0) {
        fprintf(stderr, "myOSshell: syntax error\n");
        return -1;
    }

    return 0;
}

int parse_line(char *line, ParsedLine *parsed) {
    parsed->command_count = 0;
    parsed->background = 0;

    char *trimmed = trim(line);
    if (*trimmed == '\0') {
        return 0; /* empty line, nothing to do */
    }

    size_t len = strlen(trimmed);
    if (len > 0 && trimmed[len - 1] == '&') {
        parsed->background = 1;
        trimmed[len - 1] = '\0';
        trimmed = trim(trimmed);
        if (*trimmed == '\0') {
            fprintf(stderr, "myOSshell: syntax error\n");
            return -1;
        }
    }

    char *line_copy = strdup(trimmed);
    if (!line_copy) {
        fprintf(stderr, "myOSshell: out of memory\n");
        return -1;
    }

    int count = 0;
    char *saveptr = NULL;
    char *segment = strtok_r(line_copy, "|", &saveptr);

    if (segment == NULL) {
        fprintf(stderr, "myOSshell: syntax error\n");
        free(line_copy);
        return -1;
    }

    /* detect leading/trailing pipe (e.g. "| more" or "ls |") via raw scan */
    if (trimmed[0] == '|') {
        fprintf(stderr, "myOSshell: syntax error\n");
        free(line_copy);
        return -1;
    }
    if (trimmed[strlen(trimmed) - 1] == '|') {
        fprintf(stderr, "myOSshell: syntax error\n");
        free(line_copy);
        return -1;
    }

    while (segment != NULL) {
        if (count >= MAX_COMMANDS) {
            fprintf(stderr, "myOSshell: too many pipe segments\n");
            free(line_copy);
            return -1;
        }
        char *seg_trimmed = trim(segment);
        if (*seg_trimmed == '\0') {
            fprintf(stderr, "myOSshell: syntax error\n");
            free(line_copy);
            return -1;
        }

        char *seg_dup = strdup(seg_trimmed);
        if (parse_segment(seg_dup, &parsed->commands[count]) != 0) {
            free(seg_dup);
            free(line_copy);
            return -1;
        }
        /* keep seg_dup alive; argv pointers point into it */
        parsed->commands[count]._owned = seg_dup;
        count++;
        segment = strtok_r(NULL, "|", &saveptr);
    }

    parsed->command_count = count;
    free(line_copy);
    return 0;
}

void free_parsed_line(ParsedLine *parsed) {
    for (int i = 0; i < parsed->command_count; i++) {
        if (parsed->commands[i]._owned) {
            free(parsed->commands[i]._owned);
            parsed->commands[i]._owned = NULL;
        }
    }
}
