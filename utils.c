#include"utils.h"

int open_listenfd(char* port) {
    struct addrinfo hints, *listp, *p;
    int listenfd, optval=1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
    getaddrinfo(NULL, port, &hints, &listp);
    
    // 遍历获取到的可用的addrinfo找到可用的建立socket
    for (p = listp; p; p = p->ai_next) {
        // 创建socket描述符
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; // socket()错误返回-1
        // 书中描述调用此函数用于使服务器能够被中止、重启、立即开始接受请求。
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
        // 将address与socket描述符绑定
        if ((bind(listenfd, p->ai_addr, p->ai_addrlen)) == 0)
            break; // 成功，退出循环
        if (close(listenfd) < 0) { /* Bind failed, try the next */
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    freeaddrinfo(listp);
    if (!p) /* No address worked */
        return -1;
    
    // socketfd转换为监听套接字 第二个参数代表内核接收队列最大未完成请求的数量
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
        return -1;
    }
    return listenfd;
}

int accept_connection(int listenfd) {
    int connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
    getnameinfo((struct sockaddr *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
    printf("Connected to %s:%s\n", client_hostname, client_port);
    return connfd;
}

void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;
    rio_readinitb(&rio, connfd);
    // 从connfd中读一行数据并返回ok，直到EOF
    while((n = rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("Server received %d bytes\n", (int)n);
        printf("Message: %s", buf);
        char msg[] = "ok\n";
        rio_writen(connfd, msg, strlen(msg));
    }
}

int make_socket_non_blocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) == -1)
        return -1;
}

void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}