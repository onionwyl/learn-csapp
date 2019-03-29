#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netdb.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#define	MAXLINE	 8192  /* Max text line length */
#define LISTENQ  1024  /* Second argument to listen() */

int open_listenfd(char* port);

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(0);
    }
    if((listenfd = open_listenfd(argv[1])) < 0)
        fprintf(stderr, "open_listenfd error\n");
    printf("Listening port %s\n", argv[1]);
    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        if((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen) < 0)) {
            fprintf(stderr, "accept error\n");
            exit(1);
        }
        getnameinfo((struct sockaddr *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to %s:%s\n", client_hostname, client_port);
        // echo or read something here
        // echo(connfd);
        if (close(connfd) < 0) {
            fprintf(stderr, "close error\n");
            exit(1);
        }
    }
    return 0;
}

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
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
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