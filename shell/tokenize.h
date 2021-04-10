#ifndef TOKENIZE_H
#define TOKENIZE_H

#include <stddef.h>

#include "token.h"

struct token *tokenize(char *str, size_t *n_tokens, size_t *n_execs);

#endif /* TOKENIZE_H */
