#ifndef PTHREAD_UTILS
#define PTHREAD_UTILS

#define _GNU_SOURCE
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "error_handling_utils.h"
#include <errno.h>

/* TEXIT stand for Thread EXIT*/
#define ERR_TEXIT(cond, msg)                                                   \
    if ((cond)) {                                                              \
        fprintf(stderr, msg);                                                  \
        pthread_exit((void*)EXIT_FAILURE);                                     \
    }

typedef struct tscounter {
    pthread_mutex_t mtx;
    uint val;
} tscounter_t;

/* Like pthread_create(tid, NULL, &fun, &arg) but exit the thread when get an error */
void sthread_create(pthread_t *thread, void *(*start_routine) (void *), void *arg);

/* Like pthread_join(thread, retval) but exit the thread when get an error */
void sthread_join(pthread_t thread, void **retval);

/* Like pthread_mutex_lock but exit the thread when get an error */
void mtx_lock(pthread_mutex_t *mutex);

/* Like pthread_mutex_unlock but exit the thread when get an error */
void mtx_unlock(pthread_mutex_t *mutex);

/* Like pthread_mutex_init(mutex, NULL) but exit the thread when get an error */
void mtx_init(pthread_mutex_t *mutex);

/* Like pthread_mutex_destroy(mutex) but exit the thread when get an error */
void mtx_destroy(pthread_mutex_t *mutex);

/* Like pthread_cond_init(cond, NULL) but exit the thread when get an error */
void cond_init(pthread_cond_t *cond);

/* Like pthread_cond_destroy(cond) but exit the thread when get an error */
void cond_destroy(pthread_cond_t *cond);

/* Like pthread_cond_signal(cond) but exit the thread when get an error */
void cond_signal(pthread_cond_t *cond);

/* Like pthread_cond_wait(cond, mtx) but exit the thread when get an error */
void cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx);

/* Like pthread_cond_broadcast(cond) but exit the thread when get an error */
void cond_broadcast(pthread_cond_t *cond);

/* Initialize a counter if there is an error return NULL*/
tscounter_t* counter_init(uint val);

/* Delete a counter if there is an error exit*/
void counter_del(tscounter_t *counter);

#endif
