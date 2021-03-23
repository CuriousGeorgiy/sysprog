#include "coro.h"

#include <assert.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <ucontext.h>

#include "errors.h"
#include "dynamic_memory_management.h"

/*!
 * Abstract singleton scheduler
 */
struct {
    size_t coro_pool_sz;
    coro *coro_pool;

    bool err;
    size_t semaphore;
    size_t curr_coro_id;
    struct timespec curr_coro_resume_time;
    double time_quanta;
    bool running;
} static scheduler;

static bool setup_context_stack(stack_t *stack);
static double time_elapsed_since_last_invocation();

static void coro_park();
static void coro_pass_control();

/*!
 * Sets up the coroutine scheduler
 *
 * @param coro_pool_sz   [in] number of coroutines
 * @param target_latency [in] multitasking latency
 *
 * @return true on success, false otherwise
 *
 * @note the scheduler manages an auxiliary coroutine for parking
 */
bool scheduler_setup(size_t coro_pool_sz, double target_latency)
{
    scheduler.curr_coro_id = 0;
    scheduler.semaphore = coro_pool_sz;
    scheduler.coro_pool_sz = coro_pool_sz + 1;
    scheduler.time_quanta = target_latency / coro_pool_sz;

    if ((scheduler.coro_pool = calloc(scheduler.coro_pool_sz, sizeof(*scheduler.coro_pool))) == NULL) HANDLE_ERROR("calloc: ", { goto cleanup; });

    for (size_t i = 1; i < scheduler.coro_pool_sz; ++i) {
        if (getcontext(&scheduler.coro_pool[i].ctx) != 0) HANDLE_ERROR("getcontext: ", { goto cleanup; });
        if (!setup_context_stack(&scheduler.coro_pool[i].ctx.uc_stack)) goto cleanup;
        scheduler.coro_pool[i].ctx.uc_link = &scheduler.coro_pool[0].ctx;
    }

    return true;

cleanup:
    scheduler_cleanup();

    return false;
}

/*!
 * Setups up a context stack
 *
 * @details allocates a buffer for the stack and registers it with sigaltstack
 *
 * @param stack [out]
 *
 * @return true on success, false otherwise
 */
bool setup_context_stack(stack_t *stack)
{
    assert(stack != NULL);

    if ((stack->ss_sp = calloc(SIGSTKSZ, sizeof(char))) == NULL) HANDLE_ERROR("calloc: ", { goto cleanup; });
    stack->ss_size = SIGSTKSZ;
    stack->ss_flags = 0;
    if (sigaltstack(stack, NULL) != 0) HANDLE_ERROR("sigalt_stack: ", { goto cleanup; });

    return true;

cleanup:
    free_and_null(&stack->ss_sp);

    return false;
}
/*!
 * Cleans up the scheduler
 */
void scheduler_cleanup()
{
    if (scheduler.coro_pool != NULL) {
        for (size_t i = 1; i < scheduler.coro_pool_sz; ++i) {
            free_and_null(&scheduler.coro_pool[i].ctx.uc_stack.ss_sp);
        }
    }

    free_and_null((void **) &scheduler.coro_pool);
}

/*!
 * Registers the entry point for coroutines
 *
 * @param func [in] coroutine entry point
 */
void scheduler_register_coro_entry_point(ctx_entry_point_func_t func)
{
    for (size_t i = 1; i < scheduler.coro_pool_sz; ++i) {
        makecontext(&scheduler.coro_pool[i].ctx, func, 0);
    }
}

/*!
 * Runs the scheduler, executing the coroutines and waiting for the to finish
 *
 * @return true on success, false otherwise
 */
bool scheduler_run()
{
    scheduler.running = true;
    coro_park();
    scheduler.running = false;

    return !scheduler.err;
}

/*!
 * @return current active coroutine
 */
coro *scheduler_curr_coro()
{
    return &scheduler.coro_pool[scheduler.curr_coro_id];
}

/*!
 * @return scheduler's coroutine pool
 */
coro *scheduler_coro_pool()
{
    return scheduler.coro_pool + 1;
}

/*!
 * Initially parks the scheduler's auxiliary, then all the rest of the coroutines get parked here, when they are done
 */
void coro_park()
{
    static volatile bool once = false;

    if (!once) {
        once = true;

        if (getcontext(&scheduler.coro_pool[0].ctx) != 0) HANDLE_ERROR("getcontext: ", { goto error; });
    }

    while (scheduler.semaphore) {
        coro_pass_control();
    }

error:
    scheduler.err = false;
}

/*!
 * Indicate that a coroutine is done
 */
void coro_done()
{
    coro *this = scheduler_curr_coro();
    assert(this != NULL);

    --scheduler.semaphore;
    this->done = true;
    this->exec_time += time_elapsed_since_last_invocation();
}

/*!
 * Unconditionally passes control to next coroutine
 */
void coro_suspend()
{
    if (!scheduler.running) return;

    scheduler_curr_coro()->exec_time += time_elapsed_since_last_invocation();

    coro_pass_control();
}

/*!
 * Passes control to next coroutine is the current one's time quota has been exceeded
 */
void coro_yield()
{
    if (!scheduler.running) return;

    double time_elapsed = time_elapsed_since_last_invocation();
    if (time_elapsed >= scheduler.time_quanta) {
        scheduler_curr_coro()->exec_time += time_elapsed;

        coro_pass_control();
    }
}

/*!
 * @return time elapsed since the last time control was switched to the current coroutine
 */
double time_elapsed_since_last_invocation()
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);

    return (double) (now.tv_sec - scheduler.curr_coro_resume_time.tv_sec) * pow(10, 6) +
           (double) (now.tv_nsec - scheduler.curr_coro_resume_time.tv_nsec) * pow(10, -3);
}

/*!
 * Passes control to the next coroutine
 */
void coro_pass_control()
{
    if (scheduler.err && (swapcontext(&scheduler_curr_coro()->ctx, &scheduler.coro_pool[0].ctx) != 0)) {
        HANDLE_ERROR("swapcontext: ", { exit(EXIT_FAILURE); });
    }

    ++scheduler_curr_coro()->times_passed_control;

    size_t prev_coro_id = scheduler.curr_coro_id;

    for (size_t i = 1; i < scheduler.coro_pool_sz; ++i) {
        scheduler.curr_coro_id = (scheduler.curr_coro_id + 1) % scheduler.coro_pool_sz;
        if (!scheduler_curr_coro()->done) break;
    }

    coro *this = scheduler_curr_coro();
    assert(this != NULL);

    if (timespec_get(&scheduler.curr_coro_resume_time, TIME_UTC) == 0) HANDLE_ERROR("timespec_get: ", { goto error; });
    if (swapcontext(&scheduler.coro_pool[prev_coro_id].ctx, &this->ctx) != 0) HANDLE_ERROR("calloc: ", { goto error; });

    return;

error:
    coro_error();
}

/*!
 * Sets the scheduler's semaphore to 0, causing immediate exit out of scheduler's park after passing control
 */
void coro_error()
{
    scheduler.err = true;
    scheduler.semaphore = 0;
    coro_pass_control();
}
