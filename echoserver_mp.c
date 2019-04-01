#include"server.h"

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    // pid=-1 回收所有子进程 options设为WNOHANG为非阻塞
    // 返回值=0 没有子进程可收集
    pid_t wpid;
    while((wpid = waitpid(-1, 0, WNOHANG)) > 0) {
        printf("回收的子进程pid为:%d\n",wpid);
    }
    listenfd = open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        // 子进程
        if(fork() == 0) {
            close(listenfd);
            echo(connfd);
            close(connfd);
            exit(0);
        }
        // 这里必须关闭，因为fork之后文件表中对fd的引用计数会+1，所以父进程必须关闭
        // 子进程由于得到一个连接符的副本，所以可以继续通信
        close(connfd);
    }

}