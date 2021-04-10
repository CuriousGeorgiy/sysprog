#ifndef TOKENIZE_H
#define TOKENIZE_H

#include <stddef.h>

#include "shell_errors.h"
#include "token.h"

enum shell_status tokenize(char *str, struct token **toks, size_t *n_toks, size_t *n_pipelines);

#endif /* TOKENIZE_H */
