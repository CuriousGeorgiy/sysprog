#include "dynamic_memory_management.h"

#include <assert.h>
#include <stdlib.h>

/*!
 * Frees a heap pointer and sets it to NULL
 *
 * @param heap_ptr_address [in, out]
 */
void free_and_null(void **heap_ptr_address)
{
    assert(heap_ptr_address != NULL);

    free(*heap_ptr_address);
    *heap_ptr_address = NULL;
}

