#include "analyze.h"

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "pipeline.h"
#include "shell_errors.h"

static enum shell_status analyze_cmd(struct command *cmd, const struct token **toks, const struct token *toks_end);
static size_t cnt_cmd_args(const struct token *toks, const struct token *toks_end);
static enum shell_status analyze_redir_op(struct redirection *redir, const struct token **toks, const struct token *toks_end);

enum shell_status analyze(struct list *const lst, const struct token *toks, const size_t n_toks)
{
    assert(lst != NULL);
    assert(toks != NULL);

    const struct token *const toks_end = toks + n_toks;
    size_t cmd_idx = 0;
    size_t pipeline_idx = 0;
    enum shell_status stat = SHELL_SUCCESS;
    while (toks != toks_end) {
        assert(toks != NULL);

        switch (toks->type) {
            case CMD_NAME:
                stat = analyze_cmd(&lst->pipelines[pipeline_idx].cmds[cmd_idx], &toks, toks_end);
                if (stat != SHELL_SUCCESS) return stat;

                break;
            case CMD_ARG:
                HANDLE_LOGIC_ERROR({ return false; }, "expected command name, got command argument \"%s\"\n", toks->str);

                break;
            case REDIR_OP:
                stat = analyze_redir_op(&lst->pipelines[pipeline_idx].redir, &toks, toks_end);
                if (stat != SHELL_SUCCESS) return stat;

                break;
            case REDIR_FILE_NAME:
                HANDLE_LOGIC_ERROR({ return SHELL_LOGIC_ERROR; },
                                   "expected redirection operator, got redirection file name \"%s\"\n", toks->str);

                break;
            case PIPE_OP:
                ++cmd_idx;
                ++toks;

                break;
            case LIST_OP:
                lst->pipelines[pipeline_idx].list_op = toks->op[0];
                cmd_idx = 0;
                ++pipeline_idx;
                ++toks;

                break;
            case BG_OP:
                if (++toks != toks_end) HANDLE_LOGIC_ERROR({ return SHELL_LOGIC_ERROR; },
                                                           "expected end of line after background operator\n");
                lst->bg = true;

                break;
            default:
                HANDLE_LOGIC_ERROR({ return SHELL_LOGIC_ERROR; },
                                   "unknown token type '%d'", toks->type);
        }
    }

    return SHELL_SUCCESS;
}

enum shell_status analyze_cmd(struct command *cmd, const struct token **toks, const struct token *toks_end)
{
    assert(cmd != NULL);
    assert(toks != NULL);
    assert(toks_end != NULL);

    cmd->argv = calloc(cnt_cmd_args(*toks + 1, toks_end) + 2, sizeof(*cmd->argv));
    if (cmd->argv == NULL) HANDLE_SYS_ERROR({ return SHELL_SYS_ERROR; }, "calloc: %s\n");

    cmd->argv[0] = ((*toks)++)->str;

    size_t argv_idx = 1;
    while ((*toks != toks_end) && ((*toks)->type == CMD_ARG)) {
        assert(*toks != NULL);

        cmd->argv[argv_idx++] = ((*toks)++)->str;
    }

    return SHELL_SUCCESS;
}

size_t cnt_cmd_args(const struct token *toks, const struct token *toks_end)
{
    assert(toks != NULL);
    assert(toks_end != NULL);

    size_t argc = 0;
    while ((toks != toks_end) && ((toks++)->type == CMD_ARG)) {
        assert(toks != NULL);

        ++argc;
    }

    return argc;
}

enum shell_status analyze_redir_op(struct redirection *const redir, const struct token **toks, const struct token *const toks_end)
{
    assert(redir != NULL);
    assert(toks != NULL);
    assert(*toks != NULL);
    assert(toks_end != NULL);
    assert(*toks != toks_end);

    if (*toks + 2 != toks_end) HANDLE_LOGIC_ERROR({ return SHELL_LOGIC_ERROR; }, "syntax error near unexpected token '%s'\n", (*toks)->op);

    strncpy(redir->op, ((*toks)++)->op, sizeof(redir->op));

    if ((*toks == toks_end) || (*toks)->type != REDIR_FILE_NAME) HANDLE_LOGIC_ERROR({ return SHELL_LOGIC_ERROR; },
                                                                                    "syntax error near redirection operator '%s'\n",
                                                                                    redir->op);

    redir->file_name = ((*toks)++)->str;

    return SHELL_SUCCESS;
}
