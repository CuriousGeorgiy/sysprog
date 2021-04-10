#ifndef MERGE_SORT_H
#define MERGE_SORT_H

#include <stddef.h>

#include "elem.h"

void merge_sort_array_with_coroutines(elem_t *restrict arr, elem_t *restrict aux, size_t sz);
void merge_sort_files(elem_t *restrict storage, elem_t *restrict aux, size_t storage_sz, const size_t *storage_offsets, size_t n_files);

#endif /* MERGE_SORT_H */
