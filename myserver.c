#include"utils.h"
#include"threadpool.h"
#include"epoll.h"
#include"http.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    // 连接变量
    int listenfd;
    // epoll 变量
    int epfd, count_events;
    
    listenfd = open_listenfd(argv[1]);
    make_socket_non_blocking(listenfd);
    epfd = Epoll_create(EPOLL_CLOEXEC);
    Epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, EPOLLIN|EPOLLET);
    

    threadpool_t *pool = threadpool_create(8, 100, 1000);

    while(1) {
        // 等待事件发生
        count_events = Epoll_wait(epfd, ep_events, MAXEVENTS, -1);

        // 处理事件
        handle_epoll_events(epfd, listenfd, ep_events, count_events, pool);
    }
    close(listenfd);
    close(epfd);
    return 0;

    
}