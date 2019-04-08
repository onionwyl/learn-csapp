#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<semaphore.h>

void P(sem_t *sem);
void V(sem_t *sem);

// 生产者-消费者缓冲区结构体
typedef struct {
    int *buf;          // buffer         
    int n;             // 容量
    int out;         // 消费者消费的物品id 消费后+1模n
    int in;          // 生产者生产的物品id 生产后+1模n
    sem_t mutex;       // 互斥变量
    sem_t slots;       // 记录还能生产多少东西
    sem_t items;       // 记录已经生产多少东西
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_destroy(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);
void *produce(void *vargp);
void *consume(void *vargp);

sbuf_t sbuf;
int sbuf_size = 5;

int main() {
    
    pthread_t tid;
    sbuf_init(&sbuf, sbuf_size);
    for(int i = 0; i < 10; i++) {
        pthread_create(&tid, NULL, produce, (void *)i);
    }
    for(int i = 0; i < 10; i++) {
        pthread_create(&tid, NULL, consume, (void *)i);
    }
    sleep(2);
}

void *produce(void *vargp) {
    pthread_detach(pthread_self());
    int id = (int)vargp;
    sbuf_insert(&sbuf, id);
    printf("Producer produce item %d\n", id);
}

void *consume(void *vargp) {
    pthread_detach(pthread_self());
    int id = (int)vargp;
    int item = sbuf_remove(&sbuf);
    printf("Consumer %d consume item %d\n", id, item);
}

void P(sem_t *sem) {
    if(sem_wait(sem) < 0) {
        fprintf(stderr, "P error");
        exit(0);
    }
}
void V(sem_t *sem) {
    if(sem_post(sem) < 0) {
        fprintf(stderr, "V error");
        exit(0);
    }
}

void sbuf_init(sbuf_t *sp, int n) {
    sp->buf = calloc(n, sizeof(int));
    sp->n = n;
    sp->in = sp->out = 0;
    sem_init(&sp->mutex, 0, 1);
    sem_init(&sp->slots, 0, n);
    sem_init(&sp->items, 0, 0);

}
void sbuf_destroy(sbuf_t *sp) {
    free(sp->buf);
}
// 生产者
void sbuf_insert(sbuf_t *sp, int item) {
    P(&sp->slots);   // 检查是否还可以生产
    P(&sp->mutex);   // 检查buffer互斥变量
    sp->buf[sp->in] = item; // 生产物品
    sp->in = (sp->in + 1) % sp->n;  // 编号后移
    V(&sp->mutex);  // 生产后解锁互斥量
    V(&sp->items);  // 增加剩余物品信号量
}
int sbuf_remove(sbuf_t *sp) {
    int item;
    P(&sp->items);  // 检查是否有物品
    P(&sp->mutex);  // 检查互斥量
    item = sp->buf[sp->out];    // 消费一个物品
    sp->out = (sp->out + 1) % sp->n;    // 编号后移
    V(&sp->mutex);  // 解锁互斥量
    V(&sp->slots);  // 增加可生产物品信号量
    return item;
}
