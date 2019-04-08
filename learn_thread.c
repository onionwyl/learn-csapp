#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>

void *thread(void *vargp);
char **ptr;
int main() {
    int i;
    pthread_t tid;
    char *msgs[2] = {
        "Hello from foo",  
        "Hello from bar"   
    };

    ptr = msgs; 
    for (i = 0; i < 2; i++)  
        pthread_create(&tid, NULL, thread, (void *)i); 
    pthread_exit(NULL); 
}

void *thread(void *vargp) {
    int myid = (int)vargp;
    static int cnt = 0; 
    printf("[%d]: %s (cnt=%d)\n", myid, ptr[myid], ++cnt); 
    return NULL;
}