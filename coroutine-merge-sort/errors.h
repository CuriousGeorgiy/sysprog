#ifndef ERRORS_H
#define ERRORS_H

/*!
 * Macro for handling errors
 *
 * @details prints the error description calling by calling perror and then executes code for
 * handling error
 */
#define HANDLE_ERROR(description, handle_source_code) \
do {                                                  \
    perror(description);                              \
                                                      \
    {                                                 \
        handle_source_code                            \
    }                                                 \
} while(0)

#endif /* ERRORS_H */
