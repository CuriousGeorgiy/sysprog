#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"
#include "errors.h"
#include "shell.h"

signed main()
{
    size_t buf_cap = 4096;
    char *buf = calloc(buf_cap, sizeof(*buf));
    if (buf == NULL) HANDLE_SYS_ERROR({ return EXIT_FAILURE; }, "calloc");

    size_t buf_off = 0;
    int c = 0;
    while ((c = getchar()) != EOF) {
        ungetc(c, stdin);

        if (fgets(buf + buf_off, buf_cap, stdin) == NULL) HANDLE_SYS_ERROR({ goto cleanup; }, "fgets");

        switch (buf[buf_cap - 1]) {
            case '\0':
                buf[strlen(buf) - 1] = '\0';
            case '\n':
                buf[buf_cap - 1] = '\0';
                buf_off = 0;

                shell_handle_input(buf);

                break;
            default:
                buf_off = buf_cap;
                buf_cap <<= 1;

                buf = realloc(buf, buf_cap * sizeof(*buf));
                if (buf == NULL) HANDLE_SYS_ERROR({ goto cleanup; }, "realloc");

                break;
        }
    }
    free_null((void **) &buf);

    return EXIT_SUCCESS;

cleanup:
    free_null((void **) &buf);

    return EXIT_FAILURE;
}
