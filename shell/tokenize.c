#include "tokenize.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "errors.h"
#include "token.h"

static size_t cnt_tok(const char *input);
static bool is_tok_delim(char ch);
char *parse_str(const char **str);
static void set_tok_type(struct token *tok, const struct token *prev_tok, bool first_tok);

struct token *tokenize(char *str, size_t *n_tokens, size_t *n_execs)
{
    assert(str != NULL);
    assert(n_tokens != NULL);
    assert(n_execs != NULL);

    *n_tokens = cnt_tok(str);
    struct token *tokens = calloc(*n_tokens, sizeof(*tokens));
    if (tokens == NULL) HANDLE_SYS_ERROR({ return NULL; }, "calloc");

    size_t token_idx = 0;
    *n_execs = 0;
    while ((*str != '\0') && (*str != '#')) {
        switch (*str++) {
            case ' ':
            case '\t':
            case '\n':
                break;
            case '>':
                tokens[token_idx].type = REDIR_OP;
                tokens[token_idx].redir_op[0] = '>';

                if (*str == '>') tokens[token_idx].redir_op[1] = *str;

                ++token_idx;

                break;
            case '|':
                tokens[token_idx++].type = PIPE_OP;
                ++*n_execs;

                break;
            default:
                --str;
                tokens[token_idx].str = parse_str((const char **) &str);
                set_tok_type(&tokens[token_idx], &tokens[token_idx - 1], *n_execs == 0);
                if ((tokens[token_idx++].type == EXEC_NAME) && (*n_execs == 0)) *n_execs = 1;

                break;
        }
    }

    return tokens;
}

size_t cnt_tok(const char *input)
{
    assert(input != NULL);

    if (*input == '\0') return 0;

    size_t n_toks = 0;
    char quote = '\0';
    bool eos = false;
    while (!eos && (*input != '\0') && (*input != '#')) {
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
                if (((quote == '\"') && (input[1] == '\"')) || (quote == '\0')) ++input;

                break;
            case '#':
                eos = true;

                break;
            case ' ':
            case '\t':
            case '\n':
                if (quote != '\0') break;

                n_toks += !is_tok_delim(input[-1]);

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
    return (ch == '\'') || (ch == '\"') || (ch == ' ') || (ch == '\t') || (ch == '\n');
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
                if (((quote == '\"') && ((*str)[1] == '\"')) || (quote == '\0')) ++*str;

                break;
            case '#':
                eos = true;

                break;
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

    char *res = calloc(res_len, sizeof(*res));
    if (res == NULL) HANDLE_SYS_ERROR({ return NULL; }, "calloc");

    *str = str_begin;
    size_t idx = 0;
    quote = '\0';
    eos = false;
    while (!eos && (**str != '\0')) {
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
                if ((quote == '\"') && ((*str)[1] == '\"') || (quote == '\0')) ++*str;

                break;
            case '#':
                eos = true;

                break;
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

        res[idx++] = *(*str)++;
    }

    return res;
}

void set_tok_type(struct token *tok, const struct token *prev_tok, bool first_tok)
{
    if (first_tok || (prev_tok->type == PIPE_OP)) {
        tok->type = EXEC_NAME;
    } else if (prev_tok->type == REDIR_OP) {
        tok->type = REDIR_FILE_NAME;
    } else {
        tok->type = EXEC_ARG;
    }
}
