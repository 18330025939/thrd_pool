#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H

//typedef struct thread_pool_t thread_pool_t;
typedef void (*handler_pt)(void *);

struct thread_pool_t * thread_pool_create(int thrd_count, int queue_size);

int thread_pool_destroy(struct thread_pool_t * pool);

int thread_pool_post(struct thread_pool_t *pool, handler_pt func, void *arg);

int wait_all_done(struct thread_pool_t *pool);

#endif // !__THREAD_POOL_H
