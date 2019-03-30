#include"server.h"

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

void handle(int fd) {
    
}