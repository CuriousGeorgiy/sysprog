#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>

#include "token.h"

struct token *tokenize(char *str, size_t *n_tokens, size_t *n_execs);

#endif /* TOKENIZER_H */
