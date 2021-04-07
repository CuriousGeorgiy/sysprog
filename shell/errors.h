#ifndef ERRORS_H
#define ERRORS_H

#include <stdio.h>

/*!
 * Macro for handling errors
 *
 * @details prints the error description calling by calling perror and then executes code for
 * handling error
 */
#define HANDLE_SYS_ERROR(error_handling_src_code, error_description) \
do {                                                                 \
    perror(error_description);                                       \
                                                                     \
    {                                                                \
        error_handling_src_code                                      \
    }                                                                \
} while(0)

#define HANDLE_LOGIC_ERROR(error_handling_src_code, ...) \
do {                                                     \
    fprintf(stderr, __VA_ARGS__);                        \
                                                         \
    {                                                    \
        error_handling_src_code                          \
    }                                                    \
} while (0)

#endif /* ERRORS_H */
