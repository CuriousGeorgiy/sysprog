#ifndef SHELL_H
#define SHELL_H

#include "shell_errors.h"

enum shell_status shell_handle_input(char *input);
enum shell_status shell_wait_bg_procs();

#endif /* SHELL_H */
