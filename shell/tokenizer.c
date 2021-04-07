#include "tokenizer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "errors.h"
#include "token.h"

static size_t cnt_tokens(const char *str);

struct token *tokenize(char *str, size_t *n_tokens, size_t *n_execs)
{
    assert(str != NULL);
    assert(n_tokens != NULL);
    assert(n_execs != NULL);

    *n_tokens = cnt_tokens(str);
    struct token *tokens = calloc(*n_tokens, sizeof(*tokens));
    if (tokens == NULL) HANDLE_SYS_ERROR({ return NULL; }, "calloc");

    bool in_str = false;
    size_t token_idx = 0;
    *n_execs = 0;
    while ((*str != '\0') && (*str != '#')) {
        switch (*str++) {
            case ' ':
                if (!in_str) continue;

                in_str = false;
                str[-1] = '\0';
                ++token_idx;

                break;
            case '>':
                tokens[token_idx].type = REDIR_OP;
                tokens[token_idx].redir_op[0] = '>';

                if (*str == '>') tokens[token_idx].redir_op[1] = *str++;

                ++token_idx;

                break;
            case '|':
                tokens[token_idx++].type = PIPE_OP;
                ++*n_execs;

                break;
            default:
                if (in_str) continue;

                in_str = true;
                tokens[token_idx].str = &str[-1];
                if ((token_idx == 0) || (tokens[token_idx - 1].type == PIPE_OP)) {
                    tokens[token_idx].type = EXEC_NAME;

                    if (*n_execs == 0) *n_execs = 1;
                } else if (tokens[token_idx - 1].type == REDIR_OP) {
                    tokens[token_idx].type = REDIR_FILE_NAME;
                } else {
                    tokens[token_idx].type = EXEC_ARG;
                }

                break;
        }
    }

    return tokens;
}

size_t cnt_tokens(const char *str)
{
    assert(str != NULL);

    bool in_str = false;
    size_t n_tokens = 0;
    while ((*str != '\0') && (*str != '#')) {
        switch (*str++) {
            case ' ':
                if (in_str) {
                    in_str = false;
                }

                break;
            default:
                if (!in_str) {
                    in_str = true;
                    ++n_tokens;
                }

                break;
        }
    }

    return n_tokens;
}
