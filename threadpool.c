#include"threadpool.h"

static void *threadpool_thread(void *arg);  // 线程池工作线程
static void *admin_thread(void *threadpool);    // 线程池管理线程
static int threadpool_free(threadpool_t *pool); // 释放线程池空间


threadpool_t *threadpool_create(int min_thread_num, int max_thread_num, int queue_max_size) {
    threadpool_t *pool;
    do {
        if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
            fprintf(stderr, "malloc pool error");
            break;
        }
        // 初始化线程池数据
        pool->min_thread_num = min_thread_num;
        pool->max_thread_num = max_thread_num;
        pool->live_thread_num = min_thread_num;
        pool->busy_thread_num = 0;
        pool->wait_exit_thread_num = 0;
        pool->queue_front = 0;
        pool->queue_rear = 0;
        pool->queue_size = 0;
        pool->queue_max_size = queue_max_size;
        pool->shutdown = 0;
        // 分配工作线程空间
        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * max_thread_num);
        if(pool->threads == NULL){
            fprintf(stderr, "malloc threads error");
            break;
        }
        memset(pool->threads, 0, sizeof(pthread_t) * max_thread_num);
        // 分配任务队列空间
        pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_max_size);
        // 初始化条件变量、互斥锁
        if(pthread_mutex_init(&(pool->lock), NULL) != 0  ||
            pthread_mutex_init(&(pool->thread_counter), NULL) != 0 ||
            pthread_cond_init(&(pool->queue_not_empty), NULL) != 0 ||
            pthread_cond_init(&(pool->queue_not_full), NULL) != 0) {
                fprintf(stderr, "pthread mutex init error");
                break;
            }
        // 启动min thread个线程
        for(int i = 0; i < min_thread_num; i++) {
            if(pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool) != 0) {
                fprintf(stderr, "thread create error");
                threadpool_destroy(pool);
                return NULL;
            }
            printf("Start thread 0x%x\n", (unsigned int)pool->threads[i]);
        }
        // 启动线程池管理线程
        if(pthread_create(&(pool->admin_thread), NULL, admin_thread, (void *)pool) != 0) {
            fprintf(stderr, "admin thread create error");
            threadpool_destroy(pool);
            return NULL;
        }
        return pool;
        
    }while(0);
    
    if(pool)
        threadpool_free(pool);
    return NULL;
}

static void *threadpool_thread(void *arg){
    threadpool_t *pool = (threadpool_t *)arg;
    threadpool_task_t task;
    while(1) {
        // 对线程池上锁
        pthread_mutex_lock(&(pool->lock));
        // 没有task且未停机便阻塞在这里
        while((pool->queue_size == 0) && !(pool->shutdown)) {
            // 等待条件变量，参数需要一个被上锁的互斥量，防止与其他线程产生竞争
            // 在更新条件等待队列以前，mutex保持锁定状态，并在线程挂起进入等待前解锁
            // 在条件满足从而离开pthread_cond_wait()之前，mutex将被重新加锁
            // 以与进入pthread_cond_wait()前的加锁动作对应
            // 销毁线程池时也会触发该信号量
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
            // 暂定功能 杀死多余线程
        }
        if(pool->shutdown) {
            pthread_mutex_unlock(&(pool->lock));
            printf("Thread 0x%x exit\n", (unsigned int)pthread_self());
            pthread_exit(NULL);
        }
        // 从任务队列中取出一个任务
        task.function = pool->task_queue[pool->queue_front].function;
        task.arg = pool->task_queue[pool->queue_front].arg;
        // 出队操作 队列用数组实现
        pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size;
        pool->queue_size--;
        // 信号量queue_not_full+1
        pthread_cond_broadcast(&(pool->queue_not_full));
        // 解锁pool
        pthread_mutex_unlock(&(pool->lock));
        // 执行任务
        printf("thread 0x%x start working \n", (unsigned int)pthread_self());
        // 记录正在运行的thread
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thread_num++;
        pthread_mutex_unlock(&(pool->thread_counter));
        // 调用任务函数
        (*(task.function))(task.arg);
        // 任务执行完毕
        printf("thread 0x%x end working \n", (unsigned int)pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thread_num--;
        pthread_mutex_unlock(&(pool->thread_counter));
    }
    pthread_exit(NULL);
}
// 暂定功能，动态管理线程数量
static void *admin_thread(void *threadpool){

}
int threadpool_add_task(threadpool_t *pool, void *(*function)(void *), void *arg){
    if(pool == NULL || function == NULL)
        return -1;
    if(pthread_mutex_lock(&(pool->lock)) != 0)
        return -1;
    // 任务队列满，则阻塞并等待
    while ((pool->queue_size == pool->queue_max_size) && (!pool->shutdown))
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
    // 如果线程池已关闭，则返回
    if(pool->shutdown)
        return -1;
    // 清理过去使用的队列中的arg参数
    if(pool->task_queue[pool->queue_rear].arg != NULL) {
        free(pool->task_queue[pool->queue_rear].arg);
        pool->task_queue[pool->queue_rear].arg = NULL;
    }
    // 将任务添加进队列中
    pool->task_queue[pool->queue_rear].function = function;
    pool->task_queue[pool->queue_rear].arg = arg;
    // rear同front指针计算
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;
    pool->queue_size++;
    // 使用pthread_cond_signal唤醒线程池中的一个线程
    pthread_cond_signal(&(pool->queue_not_empty));
    pthread_mutex_unlock(&(pool->lock));
    return 0;
}
// 释放线程池
int threadpool_free(threadpool_t *pool){
    if(pool == NULL)
        return -1;
    if(pool->task_queue)
        free(pool->task_queue);
    if(pool->threads){
        free(pool->threads);
        // 上锁后再销毁
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_mutex_lock(&(pool->thread_counter));
        pthread_mutex_destroy(&(pool->thread_counter));
        pthread_cond_destroy(&(pool->queue_not_empty));
        pthread_cond_destroy(&(pool->queue_not_full));
    }
    free(pool);
    pool = NULL;
    return 0;
}
// 释放线程资源
int threadpool_destroy(threadpool_t *pool){
    if(pool == NULL)
        return -1;
    pool->shutdown = 1;
    pthread_join(pool->admin_thread, NULL);
    // 通过queue_not_empty信号量通知所有线程，接下来会依次自杀
    pthread_cond_broadcast(&(pool->queue_not_empty));
    for(int i = 0; i < pool->min_thread_num; i++)
        if(pthread_join(pool->threads[i], NULL) != 0)
            printf("thread %d end failed", i);
    threadpool_free(pool);
    
    return 0;
}