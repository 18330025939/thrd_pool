#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "thread_pool.h"


pthread_mutex_t mutex;
static int nums = 0;
static int done = 0;

void do_task(void *arg)
{
    usleep(1000);
    pthread_mutex_lock(&mutex);
    done++;
    printf("doing %d tsak\n", done);
    pthread_mutex_unlock(&mutex);
}


int main(int argc, char**argv)
{

    int threads = 8;
    int queue_size = 256;

    if (argc == 2) {
        threads = atoi(argv[1]);
        if (threads <= 0) {
            printf("threads number error:%d\n", threads);
            return -1;
        }
    } else if (argc > 2) {
        threads = atoi(argv[1]);
        queue_size = atoi(argv[2]);
        if (threads <= 0 || queue_size <= 0) {
            printf("threads number or queue size error %d,%d\n", threads, queue_size);
            return -1;
        }
    }

    struct thread_pool_t *pool = thread_pool_create(threads, queue_size);
    if (pool == NULL) {
        printf("create thread_pool err\n");
        return -1;
    }

    while (thread_pool_post(pool, &do_task, NULL) == 0) {
        pthread_mutex_lock(&mutex);
        usleep(100);
        nums++;
        pthread_mutex_unlock(&mutex);
    }

    printf("add %d task\n", nums);
    wait_all_done(pool);

    printf("did %d task\n", done);
    thread_pool_destroy(pool);
    return 0;

}