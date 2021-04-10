#ifndef LIST_H
#define LIST_H

#include <stdbool.h>

struct list {
    struct pipeline *pipelines;
    bool bg;
};

#endif /* LIST_H */
