#include"server.h"
#include<sys/epoll.h>


#define EPOLL_SIZE 30

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    int epfd, count_event;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    struct epoll_event *ep_events;
    struct epoll_event event;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    listenfd = open_listenfd(argv[1]);
    // 创建epoll描述符，设置容量大小
    epfd = epoll_create(EPOLL_SIZE);
    // 分配epoll事件空间
    ep_events = (struct epoll_event*)malloc(sizeof(struct epoll_event)*EPOLL_SIZE);
    event.events = EPOLLIN;
    event.data.fd = listenfd;
    // 向epoll注册事件
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);
    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        // 等待事件发生
        count_event = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
        if(count_event == -1) {
            fprintf(stderr, "epoll_wait error");
            exit(1);
        }
        for(int i = 0; i < count_event; i++) {
            if(ep_events[i].data.fd == listenfd) {
                connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
                getnameinfo((struct sockaddr *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
                printf("Connected to %s:%s\n", client_hostname, client_port);
                // event.events = EPOLLIN;
                // event.data.fd == connfd;
                // // 添加accept描述符，等待client发送数据
                // epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);
                echo(connfd);
                close(connfd);
                
            } else {
                echo(ep_events[i].data.fd);
                close(ep_events[i].data.fd);
            }
        }
    }
    close(listenfd);
    close(epfd);
    return 0;
}