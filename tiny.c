// A simple HTTP/1.0 Web server
#include"server.h"

int main(int argc, char **argv) {
    int listenfd, connfd;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    socklen_t clientlen;    // unsigned int
    struct sockaddr_storage clientaddr;

    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    // 开启socket
    if((listenfd = open_listenfd(argv[1])) < 0) {
        fprintf(stderr, "open_listenfd error\n");
        exit(1);
    }
    printf("Listening port %s fd %d\n", argv[1], listenfd);
    // 开始处理请求 单线程
    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        if((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen)) < 0){
            fprintf(stderr, "accept error\n");
            continue;
        }
        getnameinfo((struct sockaddr *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Receive request from %s:%s\n", client_hostname, client_port);
        handle(connfd);
        if (close(connfd) < 0) {
            fprintf(stderr, "close error\n");
            exit(1);
        }
        printf("Connection closed\n");
    }
}