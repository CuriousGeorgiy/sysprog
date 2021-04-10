#include "tokenize.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "shell_errors.h"
#include "token.h"

static size_t cnt_tok(const char *input);
static bool is_tok_delim(char ch);
char *parse_str(const char **str);

enum shell_status tokenize(char *str, struct token **const toks, size_t *const n_toks, size_t *const n_pipelines)
{
    assert(str != NULL);
    assert(toks != NULL);
    assert(n_toks != NULL);

    *n_toks = cnt_tok(str);
    *toks = calloc(*n_toks, sizeof(**toks));
    if (toks == NULL) HANDLE_SYS_ERROR({ return SHELL_SYS_ERROR; }, "calloc: %s\n");

    size_t tok_idx = 0;
    *n_pipelines = 0;
    while ((*str != '\0')) {
        assert(str != NULL);

        switch (*str++) {
            case ' ':
            case '\t':
            case '\n':
                break;
            case '>':
                (*toks)[tok_idx].type = REDIR_OP;
                (*toks)[tok_idx].op[0] = '>';

                if (*str == '>') (*toks)[tok_idx].op[1] = *str++;

                ++tok_idx;

                break;
            case '|':
                if (*str == '|') {
                    (*toks)[tok_idx].type = LIST_OP;
                    (*toks)[tok_idx++].op[0] = '|';
                    ++*n_pipelines;
                    ++str;
                } else {
                    (*toks)[tok_idx++].type = PIPE_OP;
                }

                break;
            case '&':
                if (*str == '&') {
                    (*toks)[tok_idx].type = LIST_OP;
                    (*toks)[tok_idx++].op[0] = '&';
                    ++*n_pipelines;
                    ++str;
                } else {
                    (*toks)[tok_idx++].type = BG_OP;
                }

                break;
            case '#':
                return SHELL_SUCCESS;
            default:
                --str;
                (*toks)[tok_idx].str = parse_str((const char **) &str);
                if ((*n_pipelines == 0) || ((*toks)[tok_idx - 1].type == PIPE_OP) || ((*toks)[tok_idx - 1].type == LIST_OP)) {
                    (*toks)[tok_idx].type = CMD_NAME;
                } else if ((*toks)[tok_idx - 1].type == REDIR_OP) {
                    (*toks)[tok_idx].type = REDIR_FILE_NAME;
                } else {
                    (*toks)[tok_idx].type = CMD_ARG;
                }
                if (((*toks)[tok_idx].type == CMD_NAME) && (*n_pipelines == 0)) {
                    *n_pipelines = 1;
                }
                ++tok_idx;

                break;
        }
    }

    return SHELL_SUCCESS;
}

size_t cnt_tok(const char *input)
{
    assert(input != NULL);

    if (*input == '\0') return 0;

    const char *const input_begin = input;
    size_t n_toks = 0;
    char quote = '\0';
    while ((*input != '\0')) {
        assert(input != NULL);

        switch (*input) {
            case '\'':
            case '\"':
                if (quote == '\0') {
                    quote = *input;

                    break;
                }

                if (*input == quote) {
                    quote = '\0';

                    n_toks += (!!isspace(input[1]) || (input[1] == '\0'));
                }

                break;
            case '\\':
                if (((quote == '\"') && ((input[1] == '\"') || (input[1] == '\\'))) || (quote == '\0')) ++input;

                break;
            case '|':
            case '>':
            case '&':
                if (quote != '\0') break;

                if (input != input_begin) {
                    if (input[-1] != input[0]) {
                        ++n_toks;

                        if (!isspace(input[-1])) ++n_toks;
                    }
                }

                break;
            case ' ':
            case '\t':
            case '\n':
                if ((quote == '\0') && (input != input_begin)) n_toks += !is_tok_delim(input[-1]);

                break;
            case '#':
                if (quote == '\0') {
                    return n_toks + ((input != input_begin) ? !is_tok_delim(input[-1]) : 0);
                }

                break;
            default:
                break;
        }

        ++input;
    }

    return n_toks + !is_tok_delim(input[-1]);
}

bool is_tok_delim(const char ch)
{
    return (ch == '|') || (ch == '&') || (ch == '>') || (ch == '\'') || (ch == '\"') || (ch == ' ') || (ch == '\t') || (ch == '\n');
}

char *parse_str(const char **const str)
{
    assert(str != NULL);
    assert(*str != NULL);

    const char *const str_begin = *str;
    char quote = '\0';
    bool eos = false;
    size_t res_len = 0;
    while (!eos && (**str != '\0')) {
        assert(*str != NULL);

        switch (**str) {
            case '\'':
            case '\"':
                if (quote == '\0') {
                    quote = *(*str)++;

                    continue;
                }

                if (**str == quote) {
                    quote = '\0';
                    ++*str;

                    continue;
                }

                break;
            case '\\':
                if (((quote == '\"') && (((*str)[1] == '\"') || ((*str)[1] == '\\'))) || (quote == '\0')) ++*str;

                break;
            case '#':
            case '|':
            case '>':
            case '&':
            case ' ':
            case '\t':
            case '\n':
                if (quote == '\0') {
                    eos = true;

                    continue;
                }

                break;
            default:
                break;
        }

        ++res_len;
        ++*str;
    }

    char *res = calloc(res_len + 1, sizeof(*res));
    if (res == NULL) HANDLE_SYS_ERROR({ return NULL; }, "calloc: %s\n");

    *str = str_begin;
    size_t idx = 0;
    quote = '\0';
    while (**str != '\0') {
        assert(*str != NULL);

        switch (**str) {
            case '\'':
            case '\"':
                if (quote == '\0') {
                    quote = *(*str)++;

                    continue;
                }

                if (**str == quote) {
                    quote = '\0';
                    ++*str;

                    continue;
                }

                break;
            case '\\':
                if ((quote == '\"') && (((*str)[1] == '\"') || ((*str)[1] == '\\')) || (quote == '\0')) ++*str;

                break;
            case '#':
            case '|':
            case '>':
            case '&':
            case ' ':
            case '\t':
            case '\n':
                if (quote == '\0') return res;

                break;
            default:
                break;
        }

        res[idx++] = *(*str)++;
    }

    return res;
}
