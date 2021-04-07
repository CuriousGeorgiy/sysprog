#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

struct command {
    struct executable {
        char **argv;
    } *execs;

    struct redirection {
        const char *file_name;
        char op[3];
    } redir;
};

#endif /* COMMAND_H */
