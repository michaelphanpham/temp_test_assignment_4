#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    if (thread_func_args == NULL) {
        ERROR_LOG("Thread function received NULL thread_param");
        return NULL;
    }

    thread_func_args->thread_complete_success = false; // Initialize success flag

    thread_func_args->thread_id = pthread_self(); // Get the thread ID

    DEBUG_LOG("Thread %lu started with wait_to_obtain_ms: %d, wait_to_release_ms: %d",
              thread_func_args->thread_id, thread_func_args->wait_to_obtain_ms, thread_func_args->wait_to_release_ms);
    // Sleep for the specified time before obtaining the mutex
    usleep(thread_func_args->wait_to_obtain_ms * 1000); // Convert ms to us
    DEBUG_LOG("Thread %lu attempting to obtain mutex", thread_func_args->thread_id);
    // Obtain the mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Thread %lu failed to obtain mutex", thread_func_args->thread_id);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    DEBUG_LOG("Thread %lu obtained mutex", thread_func_args->thread_id);
    // Sleep for the specified time while holding the mutex
    usleep(thread_func_args->wait_to_release_ms * 1000); // Convert ms to us
    DEBUG_LOG("Thread %lu releasing mutex after holding for %d ms", thread_func_args->thread_id, thread_func_args->wait_to_release_ms);
    // Release the mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Thread %lu failed to release mutex", thread_func_args->thread_id);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }
    DEBUG_LOG("Thread %lu released mutex", thread_func_args->thread_id);
    // Mark the thread as completed successfully
    thread_func_args->thread_complete_success = true;
    // Return the thread data structure for cleanup
    DEBUG_LOG("Thread %lu completed successfully", thread_func_args->thread_id);

    // Return the thread_param for cleanup
    DEBUG_LOG("Thread %lu returning thread_param", thread_func_args->thread_id);
    thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    
     DEBUG_LOG("Starting thread to obtain mutex with wait_to_obtain_ms: %d, wait_to_release_ms: %d",
              wait_to_obtain_ms, wait_to_release_ms);
    // Allocate memory for thread_data structure
    struct thread_data *pthread_data = (struct thread_data *)malloc(sizeof(struct thread_data));
    if (pthread_data == NULL) {
        ERROR_LOG("Failed to allocate memory for thread_data");
        return false; // Memory allocation failed
    }
    pthread_data->mutex = mutex;
    pthread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    pthread_data->wait_to_release_ms = wait_to_release_ms;
    pthread_data->thread_complete_success = false;
    
    int result = pthread_create(thread, NULL, threadfunc, (void *)pthread_data);
    if (result != 0) {
        ERROR_LOG("Failed to create thread");
        return false; // Failed to create thread
    }
    // Create the thread
    return true;
}

