#ifndef SHELL_ERRORS_H
#define SHELL_ERRORS_H

#include <stdbool.h>
#include <stdio.h>

enum status {
    OK,
    ESYS,
    ELOGIC
};

/*!
 * Macro for handling errors
 *
 * @details prints the error description calling by calling perror and then executes code for
 * handling error
 */
#define HANDLE_SYS_ERROR(error_handling_src_code, error_description) \
{                                                                    \
    perror(error_description);                                       \
                                                                     \
    {                                                                \
        error_handling_src_code                                      \
    }                                                                \
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
