#include "shell.h"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>

#include "analyzer.h"
#include "errors.h"
#include "heap.h"
#include "tokenizer.h"

static void run_cmd(const struct command *cmd, size_t n_execs);
void exec(const struct command *cmd);

void shell_handle_input(char *const str)
{
    assert(str != NULL);

    size_t n_tokens = 0;
    size_t n_execs = 0;
    struct token *tokens = tokenize(str, &n_tokens, &n_execs);
    if (tokens == NULL) return;

    struct command cmd;
    if (!analyze(&cmd, tokens, n_tokens, n_execs)) goto cleanup;

    run_cmd(&cmd, n_execs);

cleanup:
    free_null((void **) &tokens);
}

void run_cmd(const struct command *const cmd, const size_t n_execs)
{
    assert(cmd != NULL);

    if (n_execs == 1) {
        exec(cmd);

        return;
    }

    int file_handle = -1;
    if (cmd->redir.op[0] == '>') {
        file_handle = open(cmd->redir.file_name, O_CREAT | O_WRONLY | ((cmd->redir.op[1] == '>') ? O_APPEND : O_TRUNC), S_IRWXU);
        if (file_handle == -1) HANDLE_SYS_ERROR({ return; }, "fopen");
    }

    int *pipes = calloc(2 * n_execs, sizeof(*pipes));
    if (pipes == NULL) HANDLE_SYS_ERROR({ goto close_file_handle; }, "calloc");
    for (size_t i = 0; i < n_execs; ++i) if (pipe(pipes + 2 * i) == -1) HANDLE_SYS_ERROR({ goto parent_cleanup; }, "pipe");

    for (size_t i = 0; i < n_execs; ++i) {
        pid_t child_pid = fork();
        if (child_pid == -1) HANDLE_SYS_ERROR({ goto parent_cleanup; }, "fork");

        if (child_pid == 0) {
            if ((file_handle != -1) && (i != n_execs - 1)) {
                if (close(file_handle) == -1) HANDLE_SYS_ERROR({ goto parent_cleanup; }, "close");
            }

            for (size_t j = 0; j < 2 * n_execs; ++j) {
                if ((j != 2 * i) && (j != 2 * (i + 1) + 1)) {
                    if (close(pipes[j]) == -1) HANDLE_SYS_ERROR({ goto parent_cleanup; }, "close");
                }
            }

            if (i == 0) {
                if (close(pipes[0]) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "close");
                if (dup2(pipes[2 * 1 + 1], STDOUT_FILENO) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "dup2");
            } else if (i == n_execs - 1) {
                if (dup2(pipes[2 * (n_execs - 1)], STDIN_FILENO) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "dup2");
                if (file_handle != -1) {
                    if (dup2(file_handle, STDOUT_FILENO) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "dup2");
                }
            } else {
                if (dup2(pipes[2 * i], STDIN_FILENO) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "dup2");
                if (dup2(pipes[2 * (i + 1) + 1], STDOUT_FILENO) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "dup2");
            }

            if (execvp(cmd->execs->argv[0], cmd->execs->argv) == -1) HANDLE_SYS_ERROR({}, "execvp");

child_cleanup:
            free_null((void **) &pipes);

            if (file_handle != -1) if (close(file_handle) == -1) HANDLE_SYS_ERROR({}, "close");

            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < n_execs; ++i) {
        if (close(pipes[2 * i]) == -1) HANDLE_SYS_ERROR({ goto parent_cleanup; }, "close");
        if (close(pipes[2 * i + 1]) == -1) HANDLE_SYS_ERROR({ goto parent_cleanup; }, "close");
    }

    int stat = 0;

    for (size_t i = 0; i < n_execs; ++i) {
        if (wait(&stat) == -1) HANDLE_SYS_ERROR({}, "wait");
    }

parent_cleanup:
    free_null((void **) &pipes);

close_file_handle:
    if (file_handle != -1) if (close(file_handle) == -1) HANDLE_SYS_ERROR({}, "close");
}

void exec(const struct command *const cmd)
{
    assert(cmd != NULL);

    int file_handle = -1;
    if (cmd->redir.op[0] == '>') {
        file_handle = open(cmd->redir.file_name, O_CREAT | O_WRONLY | ((cmd->redir.op[1] == '>') ? O_APPEND : O_TRUNC), S_IRWXU);
        if (file_handle == -1) HANDLE_SYS_ERROR({ return; }, "open");
    }

    pid_t child_pid = fork();
    if (child_pid == -1) HANDLE_SYS_ERROR({ return; }, "fork");

    if (child_pid == 0) {
        if (file_handle != -1) {
            if (dup2(file_handle, STDOUT_FILENO) == -1) HANDLE_SYS_ERROR({ exit(EXIT_FAILURE); }, "dup2");
        }

        if (execvp(cmd->execs->argv[0], cmd->execs->argv) == -1) HANDLE_SYS_ERROR({ exit(EXIT_FAILURE); }, "execvp");
    }

    int stat = 0;
    if (wait(&stat) == -1) HANDLE_SYS_ERROR({}, "wait");

    if (file_handle != -1) {
        if (close(file_handle) == -1) HANDLE_SYS_ERROR({}, "close");
    }
}

