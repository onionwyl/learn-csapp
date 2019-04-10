#include"epoll.h"

// 创建epoll描述符并初始化epoll_events空间
int Epoll_create(int flags) {
    int epfd = epoll_create(flags);
    if(epfd == -1)
        return -1;
    ep_events = (struct epoll_event*)malloc(sizeof(struct epoll_event)*MAXEVENTS);
    return epfd;
}

// 注册事件
int Epoll_ctl(int epfd, int op, int fd, int events) {
    int rc;
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    rc = epoll_ctl(epfd, op, fd, &event);
    if(rc < 0)
        unix_error("epoll ctl error");
    return rc;
}

// 等待事件发生并返回事件发生数量
int Epoll_wait(int epfd, struct epoll_event *ep_events, int max_events, int timeout) {
    int count_event = epoll_wait(epfd, ep_events, max_events, timeout);
    if(count_event == -1)
        unix_error("epoll_wait error");
    return count_event;
}

// 处理事件
void handle_epoll_events(int epfd, int listenfd, struct epoll_event *ep_events, int count_events, threadpool_t *pool) {
    for(int i = 0; i < count_events; i++) {
        int fd = ep_events[i].data.fd;
        if(fd == listenfd) {
            int connfd = accept_connection(listenfd);
            make_socket_non_blocking(connfd);
            Epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, EPOLLIN | EPOLLET | EPOLLONESHOT);
        } else {
            // 排除错误事件
            if ((ep_events[i].events & EPOLLERR) || (ep_events[i].events & EPOLLHUP)
                || (!(ep_events[i].events & EPOLLIN))){
                close(fd);
                continue;
            }
            threadpool_add_task(pool, handle, (void *)fd);
        }
    }
}