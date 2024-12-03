#include "pthread_utils.h"

/* Like pthread_create(tid, NULL, &fun, &arg) but exit the thread when get an
 * error */
inline void sthread_create(pthread_t *thread, void *(*start_routine)(void *),
                           void *arg) {
    ERR_PRINT_EXIT(pthread_create(thread, NULL, start_routine, arg) != 0,
                   "Error pthread_create\n");
}

/* Like pthread_join(thread, retval) but exit the thread when get an error */
inline void sthread_join(pthread_t thread, void **retval) {
    ERR_PRINT_EXIT(pthread_join(thread, retval) != 0, "Error pthread_join\n");
}

/* Like pthread_mutex_lock but exit the thread when get an error */
inline void mtx_lock(pthread_mutex_t *mutex) {
    ERR_TEXIT(pthread_mutex_lock(mutex) != 0, "Error pthread_mutex_lock\n");
}

/* Like pthread_mutex_unlock but exit the thread when get an error */
inline void mtx_unlock(pthread_mutex_t *mutex) {
    ERR_TEXIT(pthread_mutex_unlock(mutex) != 0, "Error pthread_mutex_unlock\n");
}

/* Like pthread_mutex_init(mutex, NULL) but exit the thread when get an error */
inline void mtx_init(pthread_mutex_t *mutex) {
    ERR_TEXIT(pthread_mutex_init(mutex, NULL) != 0,
              "Error pthread_mutex_init\n");
}

/* Like pthread_mutex_destroy(mutex) but exit the thread when get an error */
inline void mtx_destroy(pthread_mutex_t *mutex) {
    int res = pthread_mutex_destroy(mutex);
    if (res)
        errno = res;
}

/* Like pthread_cond_init(cond, NULL) but exit the thread when get an error */
inline void cond_init(pthread_cond_t *cond) {
    ERR_TEXIT(pthread_cond_init(cond, NULL) != 0, "Error pthread_cond_init\n");
}

/* Like pthread_cond_destroy(cond) but exit the thread when get an error */
inline void cond_destroy(pthread_cond_t *cond) {
    ERR_TEXIT(pthread_cond_destroy(cond) != 0, "Error pthread_cond_destroy\n");
}

/* Like pthread_cond_signal(cond) but exit the thread when get an error */
inline void cond_signal(pthread_cond_t *cond) {
    ERR_TEXIT(pthread_cond_signal(cond) != 0, "Error pthread_cond_signal\n");
}

/* Like pthread_cond_wait(cond, mtx) but exit the thread when get an error */
inline void cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx) {
    ERR_TEXIT(pthread_cond_wait(cond, mtx) != 0, "Error pthread_cond_wait\n");
}

/* Like pthread_cond_broadcast(cond) but exit the thread when get an error */
inline void cond_broadcast(pthread_cond_t *cond) {
    ERR_TEXIT(pthread_cond_broadcast(cond) != 0,
              "Error pthread_cond_broadcast\n");
}

tscounter_t *counter_init(uint val) {
    tscounter_t *c = malloc(sizeof(tscounter_t));
    ERR_RET(c == NULL, NULL);
    if (pthread_mutex_init(&c->mtx, NULL) != 0) {
        free(c);
        return NULL;
    }
    c->val = val;
    return c;
}

void counter_del(tscounter_t *counter) {
    mtx_destroy(&counter->mtx);
    if (errno != 0) {
        perror("Error pthread_mutex_destroy");
        pthread_exit((void *)EXIT_FAILURE);
    }
    free(counter);
}
