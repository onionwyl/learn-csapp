#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

typedef struct threadpool_task{
    void *(*function)(void *);
    void *arg;
}threadpool_task_t; // 线程池任务类型

typedef struct threadpool {
    pthread_mutex_t lock;   // lock the struct
    pthread_mutex_t thread_counter;
    pthread_cond_t queue_not_full; 
    pthread_cond_t queue_not_empty;

    pthread_t *threads; // 线程id数组
    pthread_t admin_thread; // 管理线程tid
    threadpool_task_t *task_queue;  //任务队列

    int min_thread_num;
    int max_thread_num;
    int live_thread_num;
    int busy_thread_num;
    int wait_exit_thread_num;

    int queue_front;
    int queue_rear;
    int queue_size;
    int queue_max_size;
    
    int shutdown;
}threadpool_t;  // 线程池类型

threadpool_t *threadpool_create(int min_thread_num, int max_thread_num, int queue_max_size);
int threadpool_add_task(threadpool_t *pool, void *(*function)(void *), void *arg);
int threadpool_destroy(threadpool_t *pool); // 关闭线程池

#endif