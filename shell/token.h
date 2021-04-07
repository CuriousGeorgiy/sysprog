#ifndef TOKEN_H
#define TOKEN_H

struct token {
    enum {
        EXEC_NAME,
        EXEC_ARG,
        REDIR_OP,
        REDIR_FILE_NAME,
        PIPE_OP
    } type;

    union {
        char *str;
        char redir_op[3];
    };
};

#endif /* TOKEN_H */
