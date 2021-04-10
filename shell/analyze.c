#include "analyzer.h"

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "command.h"
#include "errors.h"

static bool analyze_exec(struct executable *exec, const struct token **tokens, const struct token *tokens_end);
static size_t cnt_program_args(const struct token *tokens, const struct token *tokens_end);
static bool analyze_redir_op(struct redirection *redir, const struct token **tokens, const struct token *tokens_end);
static bool analyze_pipe(const struct token **tokens, const struct token *tokens_end);

bool analyze(struct command *const cmd, const struct token *tokens, const size_t n_tokens, const size_t n_execs)
{
    assert(cmd != NULL);

    cmd->execs = calloc(n_execs, sizeof(*cmd->execs));
    if (cmd->execs == NULL) HANDLE_SYS_ERROR({ return NULL; }, "calloc");

    size_t cmd_idx = 0;
    const struct token *const tokens_end = tokens + n_tokens;
    while (tokens != tokens_end) {
        assert(tokens != NULL);

        switch (tokens->type) {
            case EXEC_NAME:
                if (!analyze_exec(&cmd->execs[cmd_idx++], &tokens, tokens_end)) return false;

                break;
            case EXEC_ARG:
                HANDLE_LOGIC_ERROR({ return false; }, "expected executable name, got executable argument \"%s\"\n", tokens->str);

                break;
            case REDIR_OP:
                if (!analyze_redir_op(&cmd->redir, &tokens, tokens_end)) return false;

                break;
            case REDIR_FILE_NAME:
                HANDLE_LOGIC_ERROR({ return false; }, "expected redirection operator, got redirection file name \"%s\"\n", tokens->str);

                break;
            case PIPE_OP:
                if (!analyze_pipe(&tokens, tokens_end)) return false;

                break;
        }
    }

    return true;
}

bool analyze_exec(struct executable *const exec, const struct token **tokens, const struct token *tokens_end)
{
    assert(exec != NULL);
    assert(tokens != NULL);
    assert(tokens_end != NULL);

    exec->argv = calloc(cnt_program_args(*tokens, tokens_end) + 2, sizeof(*exec->argv));
    if (exec->argv == NULL) HANDLE_SYS_ERROR({ return false; }, "calloc");

    exec->argv[0] = ((*tokens)++)->str;

    size_t argv_idx = 1;
    while ((*tokens != tokens_end) && ((*tokens)->type == EXEC_ARG)) {
        assert(*tokens != NULL);

        exec->argv[argv_idx++] = ((*tokens)++)->str;
    }

    return true;
}

size_t cnt_program_args(const struct token *tokens, const struct token *const tokens_end)
{
    assert(tokens != NULL);
    assert(tokens_end != NULL);

    size_t argc = 0;
    while ((tokens != tokens_end) && (tokens->type == EXEC_ARG)) {
        assert(tokens != NULL);

        ++argc;
    }

    return argc;
}

bool analyze_redir_op(struct redirection *const redir, const struct token **tokens, const struct token *const tokens_end)
{
    assert(redir != NULL);
    assert(tokens != NULL);
    assert(*tokens != NULL);
    assert(tokens_end != NULL);
    assert(*tokens != tokens_end);

    if (*tokens + 2 != tokens_end) {
        HANDLE_LOGIC_ERROR({ return false; }, "syntax error near unexpected token '%s'\n", (*tokens)->redir_op);
    }

    strncpy(redir->op, ((*tokens)++)->redir_op, sizeof(redir->op));

    if ((*tokens == tokens_end) || (*tokens)->type != REDIR_FILE_NAME) {
        HANDLE_LOGIC_ERROR({ return false; }, "syntax error near redirection operator '%s'\n", redir->op);
    }

    redir->file_name = ((*tokens)++)->str;

    return true;
}

bool analyze_pipe(const struct token **tokens, const struct token *const tokens_end)
{
    assert(tokens != NULL);
    assert(*tokens != NULL);
    assert(tokens_end != NULL);
    assert(*tokens != tokens_end);

    if ((((*tokens)[-1].type != EXEC_ARG) && ((*tokens)[-1].type != EXEC_NAME)) || ((*tokens)++ == tokens_end) || ((*tokens)->type != EXEC_NAME)) {
        HANDLE_LOGIC_ERROR({ return false; }, "expected pipe operator argument\n");
    }

    return true;
}
