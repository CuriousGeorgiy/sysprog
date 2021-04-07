#include "heap.h"

#include <assert.h>
#include <stdlib.h>

/*!
 * Frees a heap pointer and sets it to NULL
 *
 * @param heap_ptr_addr [in, out]
 */
void free_null(void **heap_ptr_addr)
{
    assert(heap_ptr_addr != NULL);

    free(*heap_ptr_addr);
    *heap_ptr_addr = NULL;
}

