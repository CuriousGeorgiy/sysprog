#ifndef ANALYZER_H
#define ANALYZER_H

#include <stdbool.h>

#include "command.h"
#include "token.h"

bool analyze(struct command *cmd, const struct token *tokens, size_t n_tokens, size_t n_execs);

#endif /* ANALYZER_H */
