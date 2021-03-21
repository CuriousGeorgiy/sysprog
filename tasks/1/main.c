#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

size_t count_amount_of_numbers(FILE *file_handle);

typedef int elem_t;
void merge_sort_array(elem_t *restrict arr, elem_t *restrict aux, size_t sz);
void merge_sort_files(elem_t *restrict storage, elem_t *restrict aux, size_t storage_sz, const size_t *file_szs, size_t n_files);
void merge(elem_t *restrict writer, const elem_t *restrict reader, size_t left, size_t middle, size_t right);

signed main(int argc, const char *argv[])
{
    --argc;

    if (argc == 0) return EXIT_SUCCESS;

    FILE **input_file_handles = calloc(argc, sizeof(FILE *));
    size_t *input_file_szs = calloc(argc, sizeof(size_t));
    size_t total_amount_of_numbers = 0;

    for (size_t i = 0; i < argc; ++i) {
        if (input_file_handles[i] = fopen(argv[i + 1], "r"), input_file_handles[i] == NULL) return EXIT_FAILURE;

        input_file_szs[i] = count_amount_of_numbers(input_file_handles[i]);
        if (input_file_szs[i] == 0) return EXIT_FAILURE;

        total_amount_of_numbers += input_file_szs[i];
    }

    elem_t *storage = calloc(total_amount_of_numbers, sizeof(elem_t));

    for (size_t i = 0, j = 0, storage_offset = j; i < argc; ++i) {
        storage_offset = j;

        while (fscanf(input_file_handles[i], "%d", &storage[j]) != EOF) {
            assert(j <= total_amount_of_numbers);
            ++j;
        }
        assert(j - storage_offset == input_file_szs[i]);
        fclose(input_file_handles[i]);

        elem_t *aux = calloc(input_file_szs[i], sizeof(elem_t));
        merge_sort_array(storage + storage_offset, aux, input_file_szs[i]);
        free(aux);
    }
    free(input_file_handles);

    elem_t *aux = calloc(total_amount_of_numbers, sizeof(elem_t));
    merge_sort_files(storage, aux, total_amount_of_numbers, input_file_szs, argc);
    free(aux);
    free(input_file_szs);

    FILE *output_file_handle = fopen("result.txt", "w");
    for (size_t i = 0; i < total_amount_of_numbers; ++i) {
        fprintf(output_file_handle, (i != total_amount_of_numbers - 1) ? "%d " : "%d", storage[i]);
    }
    fclose(output_file_handle);

    free(storage);

    return EXIT_SUCCESS;
}

size_t count_amount_of_numbers(FILE *file_handle)
{
    assert(file_handle != NULL);

    size_t amount_of_numbers = 0;
    bool at_least_one_number = false;
    int ch = 0;
    while (ch = fgetc(file_handle), ch != EOF) {
        if (!at_least_one_number) at_least_one_number = true;

        if (ch == ' ') ++amount_of_numbers;
    }
    rewind(file_handle);

    return (at_least_one_number) ? amount_of_numbers + 1 : 0;
}

void merge_sort_array(elem_t *restrict arr, elem_t *restrict aux, size_t sz)
{
    assert(arr != NULL);
    assert(aux != NULL);

    elem_t *reader = aux;
    elem_t *writer = arr;

    for (size_t width = 1; width < sz; width <<= 1) {
        reader = (reader == aux) ? arr : aux;
        writer = (writer == arr) ? aux : arr;

        for (size_t i = 0; i < sz; i += 2 * width) {
            merge(writer, reader, i, min(i + width, sz),  min(i + 2 * width, sz));
        }
    }

    if (writer != arr) memcpy(arr, writer, sz * sizeof(elem_t));
}

void merge(elem_t *restrict writer, const elem_t *restrict reader, size_t left, size_t middle, size_t right)
{
    assert(writer != NULL);
    assert(reader != NULL);

    for (size_t k = left, i = left, j = middle; k < right; ++k) {
        if ((i < middle) && ((j >= right) || (reader[i] <= reader[j]))) {
            writer[k] = reader[i++];
        } else {
            writer[k] = reader[j++];
        }
    }
}

void merge_sort_files(elem_t *restrict storage, elem_t *restrict aux, size_t storage_sz, const size_t *file_szs, size_t n_files)
{
    assert(storage != NULL);
    assert(aux != NULL);
    assert(file_szs != NULL);

    elem_t *reader = aux;
    elem_t *writer = storage;

    for (size_t width = 1; width < n_files; width <<= 1) {
        reader = (reader == aux) ? storage : aux;
        writer = (writer == storage) ? aux : storage;

        for (size_t i = 0; i < n_files; i += 2 * width) {
            size_t left = 0;
            for (size_t j = 0; j < i; ++j) {
                left += file_szs[j];
            }

            size_t middle = left;
            for (size_t j = i; j < min(i + width, n_files); ++j) {
                middle += file_szs[j];
            }

            size_t right = middle;
            for (size_t j = min(i + width, n_files); j < min(i + 2 * width, n_files); ++j) {
                right += file_szs[j];
            }

            merge(writer, reader, left, middle,  right);
        }
    }

    if (writer != storage) memcpy(storage, writer, storage_sz * sizeof(elem_t));
}
