#ifndef SHELL_ERRORS_H
#define SHELL_ERRORS_H

#include <errno.h>
#include <stdio.h>
#include <string.h>

enum shell_status {
    SHELL_SUCCESS,
    SHELL_SYS_ERROR,
    SHELL_LOGIC_ERROR,
    SHELL_CHILD_PROCESS_ERROR
};

/*!
 * Macro for handling errors
 *
 * @details prints the error description calling by calling perror and then executes code for
 * handling error
 */
#define HANDLE_SYS_ERROR(error_handling_src_code, ...) \
{                                                      \
    fprintf(stderr, __VA_ARGS__, strerror(errno));     \
                                                       \
    {                                                  \
        error_handling_src_code                        \
    }                                                  \
} do {} while (false)

#define HANDLE_LOGIC_ERROR(error_handling_src_code, ...) \
{                                                        \
    fprintf(stderr, __VA_ARGS__);                        \
                                                         \
    {                                                    \
        error_handling_src_code                          \
    }                                                    \
} do {} while (false)

#endif /* SHELL_ERRORS_H */
