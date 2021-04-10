#ifndef PIPE_H
#define PIPE_H

#include <stddef.h>

struct pipe {
    struct command {
        char **argv;
    } *cmds;
    size_t n_cmds;

    struct redirection {
        const char *file_name;
        char op[3];
    } redir;

    char list_op[3];
};

#endif /* PIPE_H */
