#ifndef __EPOLL_H__
#define __EPOLL_H__

#include<sys/epoll.h>
#include"threadpool.h"
#include"utils.h"
#include"http.h"

#define MAXEVENTS 1024
struct epoll_event *ep_events;

int Epoll_create(int flags);
int Epoll_ctl(int epfd, int op, int fd, int events);
int Epoll_wait(int epfd, struct epoll_event *ep_events, int max_events, int timeout);
void handle_epoll_events(int epfd, int listenfd, struct epoll_event *ep_events, int count_events, threadpool_t *tp);

#endif