#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "heap.h"
#include "shell_errors.h"
#include "shell.h"

static char parse_quotes(const char *buf, size_t buf_sz);
static bool extend_buf(char **buf, size_t *buf_cap, size_t *buf_off);

signed main()
{
    bool error = false;

    size_t buf_cap = 4;
    char *buf = calloc(buf_cap + 1, sizeof(*buf));

    if (buf == NULL) HANDLE_SYS_ERROR({ return EXIT_FAILURE; }, "calloc: %s\n");

    size_t buf_off = 0;
    int c = 0;
    char quote = '\0';
    printf("$> ");
    while ((c = getchar()) != EOF) {
        ungetc(c, stdin);

        if (fgets(buf + buf_off, (int) (buf_cap - buf_off), stdin) == NULL) HANDLE_SYS_ERROR({
                                                                                                 error = true;
                                                                                                 goto cleanup;
                                                                                             }, "fgets: %s\n");

        size_t buf_sz = 0;
        bool buf_full = false;
        switch (buf[buf_cap - 2]) {
            case '\0':
                buf_sz = strlen(buf);

                if ((buf[0] == '\n') && (quote == '\0')) break;

                if (buf[buf_sz - 2] == '\\') {
                    buf_off = buf_sz - 2;

                    continue;
                }
            case '\n':
                buf_full = buf_sz == 0;
                buf_off = 0;

                quote = parse_quotes(buf, buf_full ? buf_cap : buf_sz);
                if (quote != '\0') {
                    if (buf_full) {
                        if (extend_buf(&buf, &buf_cap, &buf_off)) {
                            error = true;
                            goto cleanup;
                        }
                    } else {
                        buf_off = buf_sz;
                    }

                    continue;
                }

                buf[buf_full ? (buf_cap - 2) : (buf_sz - 1)] = '\0';
                buf_full = false;

                enum shell_status stat = shell_handle_input(buf);
                if (stat == SHELL_SYS_ERROR) {
                    error = true;
                    goto cleanup;
                }

                break;
            default:
                extend_buf(&buf, &buf_cap, &buf_off);

                continue;
        }

        printf("$> ");
    }

    if (shell_wait_bg_procs()) error = true;

cleanup:
    free_null((void **) &buf);

    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}

char parse_quotes(const char *buf, const size_t buf_sz)
{
    assert(buf != NULL);

    const char *const buf_end = buf + buf_sz;
    char quote = '\0';
    while ((buf != buf_end)) {
        switch (*buf) {
            case '\'':
            case '\"':
                if (quote == '\0') {
                    quote = *buf;

                    break;
                }

                if (quote == *buf) quote = '\0';

                break;
            case '\\':
                if (((quote == '\"') && ((buf[1] == '\"') || (buf[1] == '\\'))) || (quote == '\0')) ++buf;

                break;
            case '#':
                if (quote == '\0') return quote;

                break;
            default:
                break;
        }

        ++buf;
    }

    return quote;
}

bool extend_buf(char **const buf, size_t *const buf_cap, size_t *const buf_off)
{
    *buf_off = *buf_cap - 1;
    *buf_cap <<= 1;

    *buf = realloc(*buf, *buf_cap * sizeof(*buf));
    if (*buf == NULL) HANDLE_SYS_ERROR({ return true; }, "realloc: %s\n");

    (*buf)[*buf_cap - 2] = '\0';

    return false;
}


