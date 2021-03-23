#include <aio.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>

#include "coro.h"
#include "dynamic_memory_management.h"
#include "errors.h"

static void setup_coro_data(const char *file_names[], size_t n_files);
static void coroutine();
void cleanup_coro_data(size_t n_files);
static bool setup_aiocb(struct aiocb *aiocb, int fd);
static size_t str_cnt_whitespaces(const char *str);

static bool merge_sorted_files(size_t n_files, elem_t **storage_address, size_t *storage_sz);
static bool print_result(struct timespec *program_start, size_t n_files, elem_t *storage, size_t storage_sz);

signed main(signed argc, const char *argv[])
{
    struct timespec program_start;
    if (timespec_get(&program_start, TIME_UTC) == 0) HANDLE_ERROR("timespec_get: ", { return EXIT_FAILURE; });

    --argc;
    if (argc <= 1) return EXIT_FAILURE;
    double target_latency = strtod(argv[1], NULL);
    size_t n_files = argc - 1;

    if (!scheduler_setup(n_files, target_latency)) return EXIT_FAILURE;
    setup_coro_data(argv + 2, n_files);
    scheduler_register_coro_entry_point(coroutine);

    if (!scheduler_run()) goto cleanup_scheduler;

    elem_t *storage = NULL;
    size_t storage_sz = 0;
    if (!merge_sorted_files(n_files, &storage, &storage_sz)) goto cleanup_scheduler;

    if (!print_result(&program_start, n_files, storage, storage_sz)) goto cleanup;
    free_and_null((void **) &storage);
    scheduler_cleanup();

    return EXIT_SUCCESS;

cleanup:
    free_and_null((void **) &storage);

cleanup_scheduler:
    cleanup_coro_data(n_files);
    scheduler_cleanup();

    return EXIT_FAILURE;
}

/*!
 * Sets up data for the scheduler's coroutine pool
 *
 * @param file_names [in] names of input files
 * @param n_files [in] number of files
 */
void setup_coro_data(const char *file_names[], size_t n_files)
{
    assert(file_names != NULL);

    coro *coro_pool = scheduler_coro_pool();
    assert(coro_pool != NULL);

    for (size_t i = 0; i < n_files; ++i) {
        coro_pool[i].file_name = file_names[i];
    }
}

/*!
 * Coroutine for sorting individual files
 */
void coroutine()
{
    coro *this = scheduler_curr_coro();
    coro_yield();
    assert(this != NULL);
    coro_yield();

    int fd = 0;
    coro_yield();
    if ((fd = open(this->file_name, O_RDONLY)) == -1) HANDLE_ERROR("open: ", { return; });
    coro_yield();

    struct aiocb aiocb;
    coro_yield();
    if (!setup_aiocb(&aiocb, fd)) goto close_fd;
    coro_yield();
    if (aio_read(&aiocb) != 0) HANDLE_ERROR("aio_read: ", { goto close_fd; });
    coro_yield();
    int req_status = EINPROGRESS;
    coro_yield();
    while (req_status == EINPROGRESS) {
        coro_suspend();

        req_status = aio_error(&aiocb);
        coro_yield();
    }
    if (req_status != 0) HANDLE_ERROR("aio_error: ", { goto close_fd; });
    coro_yield();
    if (aio_return(&aiocb) == -1) HANDLE_ERROR("aio_return: ", { goto close_fd; });
    coro_yield();
    if (close(fd) != 0) HANDLE_ERROR("close: ", { goto close_fd; });
    coro_yield();

    this->storage_sz = str_cnt_whitespaces((const char *) aiocb.aio_buf);
    coro_yield();

    if ((this->storage = calloc(this->storage_sz, sizeof(*this->storage))) == NULL) goto close_fd;
    coro_yield();

    const char *begin = (const char *) aiocb.aio_buf;
    coro_yield();
    char *end = NULL;
    coro_yield();
    size_t i = 0;
    coro_yield();
    while (begin != end) {
        coro_yield();

        this->storage[i++] = strtol(begin, &end, 10);
        coro_yield();
        assert(i <= this->storage_sz);
        coro_yield();
        begin = end;
        coro_yield();
    }
    free_and_null((void **) &aiocb.aio_buf);
    coro_yield();

    elem_t *aux = calloc(this->storage_sz, sizeof(*aux));
    coro_yield();
    if (aux == NULL) HANDLE_ERROR("calloc: ", { goto cleanup; });
    coro_yield();
    merge_sort_array_with_coroutines(this->storage, aux, this->storage_sz);
    coro_yield();
    free_and_null((void **) &aux);
    coro_yield();

    coro_done();
    return;

close_fd:
    close(fd);

cleanup:
    free_and_null((void **) &this->storage);
    free_and_null((void **) &aiocb.aio_buf);

    coro_error();
}

/*!
 * Cleans up data of the scheduler's coroutine pool
 *
 * @param n_files
 */
void cleanup_coro_data(size_t n_files)
{
    coro *coro_pool = scheduler_coro_pool();
    assert(coro_pool != NULL);

    for (size_t i = 0; i < n_files; ++i) {
        free_and_null((void **) &coro_pool[i].storage);
    }
}

/*!
 * Setups asynchronous I/O control block (aiocb)
 *
 * @param aiocb [out]
 * @param fd [in] file descriptor to operate on
 *
 * @return true on success, false otherwise
 */
bool setup_aiocb(struct aiocb *aiocb, int fd)
{
    assert(aiocb != NULL);

    struct stat buf;
    if (fstat(fd, &buf) != 0) HANDLE_ERROR("fstat: ", { goto cleanup; });
    size_t file_sz = buf.st_size;

    aiocb->aio_fildes = fd;
    aiocb->aio_offset = 0;
    if ((aiocb->aio_buf = calloc(file_sz + 1, sizeof(*aiocb->aio_buf))) == NULL) HANDLE_ERROR("calloc: ", { goto cleanup; });
    aiocb->aio_nbytes = file_sz;
    aiocb->aio_reqprio = 0;
    aiocb->aio_sigevent.sigev_notify = SIGEV_NONE;

    return true;

cleanup:
    free_and_null((void **) &aiocb->aio_buf);

    return false;
}

/*!
 * Counts the number of whitespaces in string
 *
 * @param str [in]
 *
 * @return number of whitespaces found in string
 */
size_t str_cnt_whitespaces(const char *str)
{
    assert(str != NULL);

    size_t amount_of_numbers = 0;
    bool at_least_one_number = false;
    const char *restrict reader = str;
    while (*reader != '\0') {
        if (!at_least_one_number) at_least_one_number = true;
        if (*(reader++) == ' ') ++amount_of_numbers;
    }

    return at_least_one_number ? amount_of_numbers + 1 : 0;
}

/*!
 * Merges sorted arrays containing numbers from input files into one sorted array
 *
 * @param n_files         [in] number of input files
 * @param storage_address [out] pointer to an array, expected to be NULL
 * @param storage_sz      [out]
 *
 * @return true on success, false otherwise
 *
 * @attention dynamically allocates storage, therefore the caller is responsible for freeing the storage
 */
bool merge_sorted_files(size_t n_files, elem_t **storage_address, size_t *storage_sz)
{
    assert(storage_address != NULL);
    assert(*storage_address == NULL);
    assert(storage_sz != NULL);

    coro *coro_pool = scheduler_coro_pool();
    assert(coro_pool != NULL);

    *storage_sz = 0;
    size_t *storage_offsets = calloc(n_files, sizeof(*storage_offsets));
    if (storage_offsets == NULL) HANDLE_ERROR("calloc: ", { return false; });

    for (size_t i = 0; i < n_files; ++i) {
        storage_offsets[i] = *storage_sz;
        *storage_sz += coro_pool[i].storage_sz;
    }

    if ((*storage_address = calloc(*storage_sz, sizeof(**storage_address))) == NULL) HANDLE_ERROR("calloc: ", { goto cleanup; });

    for (size_t i = 0; i < n_files; ++i) {
        memcpy(*storage_address + storage_offsets[i], coro_pool[i].storage, coro_pool[i].storage_sz);
    }
    cleanup_coro_data(n_files);

    elem_t *aux = calloc(*storage_sz, sizeof(*aux));
    if (aux == NULL) HANDLE_ERROR("calloc: ", { goto cleanup; });
    merge_sort_files(*storage_address, aux, *storage_sz, storage_offsets, n_files);
    free_and_null((void **) &storage_offsets);
    free_and_null((void **) &aux);

    return true;

cleanup:
    free_and_null((void **) &storage_offsets);
    free_and_null((void **) storage_address);

    return false;
}

/*!
 * Prints program execution details, prints result of sorting contents of input files to 'result.txt'.
 *
 * @param program_start [in] program execution start timestamp
 * @param n_files       [in] number of input files
 * @param storage       [in] array containing sorted numbers from input file
 * @param storage_sz    [in]
 *
 * @return true on success, false otherwise
 */
bool print_result(struct timespec *program_start, size_t n_files, elem_t *storage, size_t storage_sz)
{
    assert(program_start != NULL);

    coro *coro_pool = scheduler_coro_pool();
    assert(coro_pool != NULL);

    struct timespec now;
    if (timespec_get(&now, TIME_UTC) == 0) HANDLE_ERROR("timespec_get: ", { return false; });

    for (size_t i = 0; i < n_files; ++i) {
        printf("Coroutine #%zu execution time: %lg microseconds\n"
               "Coroutine #%zu passed control: %u times\n"
               "\n",
               i + 1, coro_pool[i].exec_time,
               i + 1, coro_pool[i].times_passed_control);
    }

    printf("Total execution time: %lg microseconds\n",
           (double) (now.tv_sec - program_start->tv_sec) * pow(10, 6) + (double) (now.tv_nsec - program_start->tv_nsec) * pow(10, -3));

    FILE *output_file_handle = fopen("result.txt", "w");
    if (output_file_handle == NULL) HANDLE_ERROR("fopen: ", { return false; });
    for (size_t i = 0; i < storage_sz; ++i) {
        fprintf(output_file_handle, (i != storage_sz - 1) ? "%d " : "%d", storage[i]);
    }
    fclose(output_file_handle);

    return true;
}
