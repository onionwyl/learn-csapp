#include"threadpool.h"
#include<unistd.h>
void *do_process(void *arg) {
    int num = (int)arg;
    printf("Process %d is running\n", num);
}


int main() {
    threadpool_t *pool = threadpool_create(10, 100, 100);
    for(int i = 0; i < 10; i++) {
        threadpool_add_task(pool, do_process, (void *)i);
    }
    sleep(2);
    for(int i = 0; i < 20; i++) {
        threadpool_add_task(pool, do_process, (void *)i);
    }
    sleep(2);
    printf("destroy\n");
    threadpool_destroy(pool);
    sleep(2);
}