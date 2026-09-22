#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

static bool sleep_ms(int milliseconds)
{
    struct timespec remaining = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = (long)(milliseconds % 1000) * 1000000L
    };

    while (nanosleep(&remaining, &remaining) != 0) {
        if (errno != EINTR) {
            return false;
        }
    }

    return true;
}

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    bool mutex_locked = false;

    thread_func_args->thread_complete_success = false;

    if (!sleep_ms(thread_func_args->wait_to_obtain_ms)) {
        ERROR_LOG("Unable to complete wait before obtaining mutex");
        return thread_param;
    }

    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Unable to obtain mutex");
        return thread_param;
    }
    mutex_locked = true;

    if (!sleep_ms(thread_func_args->wait_to_release_ms)) {
        ERROR_LOG("Unable to complete wait before releasing mutex");
    } else {
        thread_func_args->thread_complete_success = true;
    }

    if (mutex_locked && pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Unable to release mutex");
        thread_func_args->thread_complete_success = false;
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data *thread_func_args;

    if (thread == NULL || mutex == NULL ||
        wait_to_obtain_ms < 0 || wait_to_release_ms < 0) {
        return false;
    }

    thread_func_args = malloc(sizeof(*thread_func_args));
    if (thread_func_args == NULL) {
        return false;
    }

    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;
    thread_func_args->thread_complete_success = false;

    if (pthread_create(thread, NULL, threadfunc, thread_func_args) != 0) {
        free(thread_func_args);
        return false;
    }

    return true;
}
