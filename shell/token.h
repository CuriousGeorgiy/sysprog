#ifndef TOKEN_H
#define TOKEN_H

struct token {
    enum {
        CMD_NAME,
        CMD_ARG,
        REDIR_OP,
        REDIR_FILE_NAME,
        PIPE_OP,
        LIST_OP,
        BG_OP
    } type;

    union {
        char *str;
        char op[3];
    };
};

#endif /* TOKEN_H */
