#include "shell.h"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>

#include "analyze.h"
#include "shell_errors.h"
#include "heap.h"
#include "tokenize.h"

struct shell {
    size_t n_bg_procs;
} static sh;

static enum shell_status exec_list(const struct list *lst);
enum shell_status exec_pipe(const struct pipeline *pipeline);
enum shell_status exec_single_cmd(const struct pipeline *cmd, int fd);

enum shell_status shell_handle_input(char *const input)
{
    assert(input != NULL);

    size_t n_toks = 0;
    size_t n_pipelines = 0;
    struct token *toks = NULL;
    enum shell_status stat = tokenize(input, &toks, &n_toks, &n_pipelines);
    if (stat != SHELL_SUCCESS) return stat;
    if (n_pipelines == 0) return SHELL_SUCCESS;

    struct list lst = {.pipelines = NULL, .bg = false};
    lst.pipelines = calloc(n_pipelines, sizeof(*lst.pipelines));
    if (lst.pipelines == NULL) HANDLE_SYS_ERROR({
                                                    stat = SHELL_SYS_ERROR;
                                                    goto cleanup_toks;
                                                }, "calloc: %s\n");

    size_t pipeline_idx = 0;
    for (size_t i = 0; i < n_toks; ++i) {
        if (toks[i].type == CMD_NAME) ++lst.pipelines[pipeline_idx].n_cmds;
        if (toks[i].type == LIST_OP) ++pipeline_idx;
    }
    for (size_t i = 0; i < n_pipelines; ++i) {
        lst.pipelines[i].cmds = calloc(lst.pipelines[i].n_cmds, sizeof(*lst.pipelines->cmds));
        if (lst.pipelines[i].cmds == NULL) HANDLE_SYS_ERROR({ goto cleanup_lst; }, "calloc: %s\n");
    }

    stat = analyze(&lst, toks, n_toks);
    if (stat != SHELL_SUCCESS) goto cleanup_toks;

    if (lst.bg) {
        ++sh.n_bg_procs;

        pid_t child_pid = fork();
        if (child_pid == -1) HANDLE_SYS_ERROR({ return false; }, "fork: %s\n");

        if (child_pid == 0) {
            exec_list(&lst);

            exit(EXIT_SUCCESS);
        }

        goto cleanup_lst;
    }

    exec_list(&lst);

cleanup_lst:
    for (size_t i = 0; i < n_pipelines; ++i) free_null((void **) lst.pipelines[i].cmds);
    free_null((void **) lst.pipelines);
cleanup_toks:
    for (size_t i = 0; i < n_toks; ++i) {
        if ((toks[i].type == CMD_NAME) || (toks[i].type == CMD_ARG)) free_null((void **) &toks[i].str);
    }

    free_null((void **) &toks);

    return stat;
}

enum shell_status exec_list(const struct list *const lst)
{
    assert(lst != NULL);

    enum shell_status stat = SHELL_SUCCESS;
    size_t lst_idx = 0;
    do {
        if (lst_idx == 0) {
            stat = exec_pipe(lst->pipelines);

            continue;
        }

        switch (lst->pipelines[lst_idx - 1].list_op) {
            case '|':
                if (stat == SHELL_SUCCESS) {
                    continue;
                } else if (stat == SHELL_CHILD_PROCESS_ERROR) {
                    stat = exec_pipe(&lst->pipelines[lst_idx]);
                } else {
                    return stat;
                }

                break;
            case '&':
                if (stat != SHELL_SUCCESS) {
                    if (stat == SHELL_CHILD_PROCESS_ERROR) {
                        continue;
                    } else {
                        return stat;
                    }
                } else {
                    stat = exec_pipe(&lst->pipelines[lst_idx]);
                }

                break;
            default:
                HANDLE_LOGIC_ERROR({ return SHELL_LOGIC_ERROR; }, "unknown list operator '%c'", lst->pipelines[-1].list_op);
        }
    } while (lst->pipelines[lst_idx++].list_op != '\0');

    return SHELL_SUCCESS;
}

enum shell_status exec_pipe(const struct pipeline *const pipeline)
{
    assert(pipeline != NULL);

    int fd = -1;
    if (pipeline->redir.op[0] == '>') {
        fd = open(pipeline->redir.file_name, O_CREAT | O_WRONLY | ((pipeline->redir.op[1] == '>') ? O_APPEND : O_TRUNC), S_IRWXU);
        if (fd == -1) HANDLE_SYS_ERROR({ return SHELL_SYS_ERROR; }, "fopen: %s\n");
    }

    if (pipeline->n_cmds == 1) return exec_single_cmd(pipeline, fd);

    enum shell_status stat = SHELL_SUCCESS;

    int *pipeline_fds = calloc(2 * pipeline->n_cmds, sizeof(*pipeline_fds));
    if (pipeline_fds == NULL) HANDLE_SYS_ERROR({
                                               stat = SHELL_SYS_ERROR;
                                               goto close_file_handle;
                                           }, "calloc: %s\n");
    for (size_t i = 0; i < pipeline->n_cmds; ++i)
        if (pipe(pipeline_fds + 2 * i) == -1) HANDLE_SYS_ERROR({
                                                               stat = SHELL_SYS_ERROR;
                                                               goto parent_cleanup;
                                                           }, "pipeline: %s\n");

    pid_t last_child_pid = 0;
    for (size_t i = 0; i < pipeline->n_cmds; ++i) {
        pid_t child_pid = fork();
        if (child_pid == -1) HANDLE_SYS_ERROR({
                                                  stat = SHELL_SYS_ERROR;
                                                  goto parent_cleanup;
                                              }, "fork: %s\n");

        if (child_pid == 0) {
            if ((fd != -1) && (i != pipeline->n_cmds - 1)) {
                if (close(fd) == -1) HANDLE_SYS_ERROR({
                                                          stat = SHELL_SYS_ERROR;
                                                          goto parent_cleanup;
                                                      }, "close: %s\n");
            }

            for (size_t j = 0; j < 2 * pipeline->n_cmds; ++j) {
                if ((j != 2 * i) && (j != 2 * (i + 1) + 1)) {
                    if (close(pipeline_fds[j]) == -1) HANDLE_SYS_ERROR({
                                                                       stat = SHELL_SYS_ERROR;
                                                                       goto parent_cleanup;
                                                                   }, "close: %s\n");
                }
            }

            if (i == 0) {
                if (close(pipeline_fds[0]) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "close: %s\n");
                if (dup2(pipeline_fds[2 * 1 + 1], STDOUT_FILENO) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "dup2: %s\n");
            } else if (i == pipeline->n_cmds - 1) {
                if (dup2(pipeline_fds[2 * (pipeline->n_cmds - 1)], STDIN_FILENO) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "dup2: %s\n");
                if (fd != -1) {
                    if (dup2(fd, STDOUT_FILENO) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "dup2: %s\n");
                }
            } else {
                if (dup2(pipeline_fds[2 * i], STDIN_FILENO) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "dup2: %s\n");
                if (dup2(pipeline_fds[2 * (i + 1) + 1], STDOUT_FILENO) == -1) HANDLE_SYS_ERROR({ goto child_cleanup; }, "dup2: %s\n");
            }

            if (execvp(pipeline->cmds[i].argv[0], pipeline->cmds[i].argv) == -1) HANDLE_SYS_ERROR({}, "execvp '%s': %s\n",
                                                                                                  pipeline->cmds[i].argv[0]);

child_cleanup:
            free_null((void **) &pipeline_fds);

            if (fd != -1) if (close(fd) == -1) HANDLE_SYS_ERROR({ exit(EXIT_FAILURE); }, "close: %s\n");
        }

        if (i == pipeline->n_cmds - 1) last_child_pid = child_pid;
    }

    bool last_cmd_error = true;

    for (size_t i = 0; i < pipeline->n_cmds; ++i) {
        if (close(pipeline_fds[2 * i]) == -1) HANDLE_SYS_ERROR({
                                                               stat = SHELL_SYS_ERROR;
                                                               goto parent_cleanup;
                                                           }, "close: %s\n");
        if (close(pipeline_fds[2 * i + 1]) == -1) HANDLE_SYS_ERROR({
                                                                   stat = SHELL_SYS_ERROR;
                                                                   goto parent_cleanup;
                                                               }, "close: %s\n");
    }

    int child_stat = 0;
    for (size_t i = 0; i < pipeline->n_cmds; ++i) {
        pid_t child_pid = wait(&child_stat);
        if (child_pid == -1) HANDLE_SYS_ERROR({
                                                  stat = SHELL_SYS_ERROR;
                                                  continue;
                                              }, "wait: %s\n");

        if (child_pid == last_child_pid) {
            if (!WIFEXITED(child_stat)) HANDLE_LOGIC_ERROR({
                                                               stat = SHELL_CHILD_PROCESS_ERROR;
                                                               continue;
                                                           }, "child did not terminate normally");

            last_cmd_error = WEXITSTATUS(child_stat);
        }
    }

parent_cleanup:
    free_null((void **) &pipeline_fds);

close_file_handle:
    if (fd != -1) if (close(fd) == -1) HANDLE_SYS_ERROR({ return SHELL_SYS_ERROR; }, "close: %s\n");

    if (stat != SHELL_SUCCESS) return stat;

    return last_cmd_error ? SHELL_CHILD_PROCESS_ERROR : SHELL_SUCCESS;
}

enum shell_status exec_single_cmd(const struct pipeline *const cmd, int fd)
{
    assert(cmd != NULL);

    pid_t child_pid = fork();
    if (child_pid == -1) HANDLE_SYS_ERROR({ return SHELL_SYS_ERROR; }, "fork: %s\n");

    if (child_pid == 0) {
        if (fd != -1) {
            if (dup2(fd, STDOUT_FILENO) == -1) HANDLE_SYS_ERROR({ exit(EXIT_FAILURE); }, "dup2: %s\n");
        }

        if (execvp(cmd->cmds->argv[0], cmd->cmds->argv) == -1) HANDLE_SYS_ERROR({ exit(EXIT_FAILURE); }, "execvp '%s': %s\n",
                                                                                cmd->cmds->argv[0]);
    }

    enum shell_status stat = SHELL_SUCCESS;

    int child_stat = 0;
    if (wait(&child_stat) == -1) HANDLE_SYS_ERROR({ stat = SHELL_SYS_ERROR; }, "wait: %s\n");

    if (fd != -1) {
        if (close(fd) == -1) HANDLE_SYS_ERROR({ return SHELL_SYS_ERROR; }, "close: %s\n");
    }

    if (stat != SHELL_SUCCESS) return stat;

    if (!WIFEXITED(child_stat)) HANDLE_LOGIC_ERROR({ return SHELL_CHILD_PROCESS_ERROR; }, "child did not terminate normally");

    return WEXITSTATUS(child_stat) ? SHELL_CHILD_PROCESS_ERROR : SHELL_SUCCESS;
}

enum shell_status shell_wait_bg_procs()
{
    enum shell_status stat = SHELL_SUCCESS;
    for (size_t i = 0; i < sh.n_bg_procs; ++i) {
        pid_t child_pid = wait(NULL);
        if (child_pid == -1) HANDLE_SYS_ERROR({ stat = SHELL_SYS_ERROR; }, "wait: %s\n");
    }

    return stat;
}
