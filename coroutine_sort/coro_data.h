#ifndef CORO_DATA_H
#define CORO_DATA_H

#define CORO_DATA struct { \
    const char *file_name; \
    elem_t *storage;       \
    size_t storage_sz;     \
}

#endif /* CORO_DATA_H */
