#ifndef PIPELINE_H
#define PIPELINE_H

#include <stddef.h>

struct pipeline {
    struct command {
        char **argv;
    } *cmds;
    size_t n_cmds;

    struct redirection {
        const char *file_name;
        char op[3];
    } redir;

    char list_op;
};

#endif /* PIPELINE_H */
