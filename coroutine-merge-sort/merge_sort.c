#include "merge_sort.h"

#include <assert.h>
#include <string.h>

#include "coro.h"

static size_t min_idx(size_t a, size_t b);
static void merge(elem_t *restrict writer, const elem_t *restrict reader, size_t left, size_t middle, size_t right);

/*!
 * Implementation of bottom-top merge sort with coroutines
 *
 * @param arr [in, out] array to sort
 * @param aux [in, out] auxiliary array, expected to be at least the same size as arr
 * @param sz [in] size of arrays
 *
 * @note sorting result in stored in arr
 */
void merge_sort_array_with_coroutines(elem_t *restrict arr, elem_t *restrict aux, size_t sz)
{
    assert(arr != NULL);
    assert(aux != NULL);

    elem_t *reader = aux;
    coro_yield();
    elem_t *writer = arr;
    coro_yield();

    for (size_t width = 1; width < sz; width <<= 1) {
        coro_yield();
        reader = (reader == aux) ? arr : aux;
        coro_yield();
        writer = (writer == arr) ? aux : arr;
        coro_yield();

        for (size_t i = 0; i < sz; i += 2 * width) {
            coro_yield();
            merge(writer, reader, i, min_idx(i + width, sz), min_idx(i + 2 * width, sz));
            coro_yield();
        }
    }

    if (writer != arr) {
        coro_yield();
        memcpy(arr, writer, sz * sizeof(elem_t));
        coro_yield();
    }
}

/*!
 * Implementation of a custom bottom-top merge sort, operating on a sequence of sorted arrays of numbers from files
 *
 * @param storage [in, out] sequence of sorted arrays merged together
 * @param aux [in, out] auxiliary array, expected to be at least the same size as storage
 * @param storage_sz [in]
 * @param storage_offsets [in] array of offsets for sorted arrays
 * @param n_files [in] number of files (corresponding to the number of sorted arrays)
 *
 * @note sorting result is stored in storage
 */
void merge_sort_files(elem_t *restrict storage, elem_t *restrict aux, size_t storage_sz, const size_t *storage_offsets, size_t n_files)
{
    assert(storage != NULL);
    assert(aux != NULL);
    assert(storage_offsets != NULL);

    elem_t *reader = aux;
    elem_t *writer = storage;

    for (size_t width = 1; width < n_files; width <<= 1) {
        reader = (reader == aux) ? storage : aux;
        writer = (writer == storage) ? aux : storage;

        for (size_t i = 0; i < n_files; i += 2 * width) {
            merge(writer, reader, storage_offsets[i], storage_offsets[min_idx(i + width, n_files - 1)],
                  storage_offsets[min_idx(i + 2 * width, n_files - 1)]);
        }
    }

    if (writer != storage) {
        memcpy(storage, writer, storage_sz * sizeof(elem_t));
    }
}

/*!
 * @param a [in] first index
 * @param b [in] second index
 *
 * @return minimum of two indices
 */
static size_t min_idx(size_t a, size_t b)
{
    return (a <= b) ? a : b;
}

/*!
 * Merges two sorted subarrays
 *
 * @param writer [out] array to which the merge result is written to
 * @param reader [in] array containing the subarrays
 * @param left [in] left bound for first subarray (inclusive)
 * @param middle [in] right bound for first subarray (exclusive), left bound for second subarray (inclusive)
 * @param right [in] right bound for second subarray (exclusive)
 */
void merge(elem_t *restrict writer, const elem_t *restrict reader, size_t left, size_t middle, size_t right)
{
    assert(writer != NULL);
    assert(reader != NULL);

    for (size_t k = left, i = left, j = middle; k < right; ++k) {
        coro_yield();
        if ((i < middle) && ((j >= right) || (reader[i] <= reader[j]))) {
            coro_yield();
            writer[k] = reader[i++];
            coro_yield();
        } else {
            coro_yield();
            writer[k] = reader[j++];
            coro_yield();
        }
    }
}
