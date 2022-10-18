#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include "thread_pool.h"

typedef struct task_t {

    handler_pt func;
    void *arg;

}task_t;

typedef struct task_queue_t {
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    task_t *queue;

}task_queue_t;


struct thread_pool_t {

    pthread_mutex_t mutex;
    pthread_cond_t condition;
    pthread_t *threads;
    task_queue_t task_queue;

    int closed;
    int started;

    int thrd_count;
    int queue_size;

};

static int thread_pool_free(struct thread_pool_t *pool);

void *thread_worker(void *arg) 
{
    struct thread_pool_t *pool = (struct thread_pool_t *)arg;
    task_queue_t *queue;
    task_t task;

    for(;;) {

        pthread_mutex_lock(&pool->mutex);
        queue = &pool->task_queue;
        //虚假唤醒
        //1.可能被信号唤醒
        //2.业务场景造成
        while (pool->closed == 0 && queue->count == 0) { //关闭或者产生任务
            //先释放 mutex
            //阻塞在condition （线程休眠了）
            // ======================
            //解除阻塞
            //获取 mutex
            pthread_cond_wait(&pool->condition, &pool->mutex);

        }
        if (pool->closed == 1)
            break;
        
        task = queue->queue[queue->head];
        queue->head = (queue->head + 1) % pool->queue_size;
        queue->count --;
        pthread_mutex_unlock(&pool->mutex);

        (*(task.func))(task.arg);

    }
    printf("pthread_id %lu\n", pthread_self());
    pool->started --;
    pthread_mutex_unlock(&pool->mutex);
    pthread_exit(NULL);
    return NULL;

}

struct thread_pool_t* thread_pool_create(int thrd_count, int queue_size)
{
    struct thread_pool_t * pool;

    if (thrd_count <= 0 && queue_size <= 0)
        return  NULL;

    pool = (struct thread_pool_t *)malloc(sizeof(*pool));
    if (pool == NULL) {

        return NULL;
    }
    pool->closed = pool->started = 0;
    pool->thrd_count = 0;
    pool->queue_size = queue_size;
    
    pool->task_queue.head = pool->task_queue.tail = 0;
    pool->task_queue.count = 0;
    pool->task_queue.queue = (task_t *)malloc(sizeof(task_t) * queue_size);
    if (pool->task_queue.queue == NULL) {

        thread_pool_free(pool);
        return NULL;
    }

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thrd_count);
    if (pool->threads == NULL) {
        
        // free(pool->task_queue.queue);
        thread_pool_free(pool);
        return NULL;
    }

    int i = 0;
    for (; i < thrd_count; i++) {

        if (pthread_create(&pool->threads[i], NULL, thread_worker, (void *)pool) != 0) {

            thread_pool_free(pool);    
            return NULL;
        }

        pool->started ++;
        pool->thrd_count ++;
    }
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->condition, NULL);
    
    return pool;
}
int wait_all_done(struct thread_pool_t *pool)
{
    int i = 0, ret = 0;
    
    for (; i < pool->thrd_count; i++) {

        if (pthread_join(pool->threads[i], NULL) != 0)
            ret = -1;

    }

    return ret;


}

static int thread_pool_free(struct thread_pool_t *pool)
{

    if (pool == NULL || pool->started == 0)
        return 0;

    if (pool->threads) {
        free(pool->threads);
        pool->threads = NULL;

        pthread_mutex_lock(&pool->mutex);
        pthread_mutex_destroy(&pool->mutex);
        pthread_cond_destroy(&pool->condition);
    }

    if (pool->task_queue.queue) {
        free(pool->task_queue.queue);
        pool->task_queue.queue = NULL;
    }
    
    free(pool);

    return 0;
}
int thread_pool_destroy(struct thread_pool_t * pool)
{

    if (pool == NULL)
        return -1;

    if (pthread_mutex_lock(&pool->mutex) != 0)
        return -2;

    if (pool->closed == 1)
        return -3;
    
    pool->closed = 1;

    if (pthread_cond_broadcast(&pool->condition) != 0 ||
        pthread_mutex_unlock(&pool->mutex) != 0)
        return -2;

    wait_all_done(pool);

    thread_pool_free(pool);
    return 0;
}

int thread_pool_post(struct thread_pool_t *pool, handler_pt func, void *arg)
{
    if (pool == NULL || func == NULL)
        return -1;

    if (pthread_mutex_lock(&pool->mutex) != 0)
        return -1;
    
    if (pool->closed == 1 || pool->queue_size == pool->task_queue.count) {
        pthread_mutex_unlock(&pool->mutex);
        return -2;
    }


    task_queue_t *task_queue = &(pool->task_queue);
    task_t *task = &(task_queue->queue[task_queue->tail]);
    task->func = func;
    task->arg = arg;
    task_queue->tail = (task_queue->tail + 1) % pool->queue_size;
    task_queue->count ++;
    printf("task_queue->count %d\n",task_queue->count);
    if (pthread_cond_signal(&pool->condition) != 0) {
        pthread_mutex_unlock(&pool->mutex);
        return -2;
    }

    pthread_mutex_unlock(&pool->mutex);

    return 0;
}