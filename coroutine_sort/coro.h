#ifndef CORO_H
#define CORO_H

#include <stdbool.h>
#include <stdio.h>
#include <ucontext.h>

#include "merge_sort.h"

/*!
 * Alias for context entry point function
 */
typedef void (*ctx_entry_point_func_t)(void);

#include "coro_data.h"
#ifndef CORO_DATA
#err "the parameters that coroutines receive must be specified by defining an anonymous struct as CORO_DATA in coro_data.h"
#endif

/*!
 * Abstract coroutine which the scheduler is based on
 */
typedef struct {
    ucontext_t ctx;
    unsigned times_passed_control;
    double exec_time;
    bool done;

    CORO_DATA;
} coro;

bool scheduler_setup(size_t coro_pool_sz, double target_latency);
void scheduler_cleanup();
void scheduler_register_coro_entry_point(ctx_entry_point_func_t func);
bool scheduler_run();
coro *scheduler_curr_coro();
coro *scheduler_coro_pool();

void coro_error();
void coro_done();
void coro_suspend();
void coro_yield();

#endif /* CORO_H */
