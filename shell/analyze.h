#ifndef ANALYZE_H
#define ANALYZE_H

#include <stdbool.h>

#include "shell_errors.h"
#include "list.h"
#include "pipeline.h"
#include "token.h"

enum shell_status analyze(struct list *lst, const struct token *toks, size_t n_toks);

#endif /* ANALYZE_H */
