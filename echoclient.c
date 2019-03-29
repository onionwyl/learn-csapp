#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netdb.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#include"rio.h"

#define	MAXLINE	 8192  /* Max text line length */

int open_clientfd(char *hostname, char *port);

int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if(argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    if((clientfd = open_clientfd(host, port)) < 0) {
        fprintf(stderr, "open clientfd error");
        exit(0);
    }
    rio_readinitb(&rio, clientfd);
    while(fgets(buf, MAXLINE, stdin) != NULL) {
        rio_writen(clientfd, buf, strlen(buf));
        rio_readlineb(&rio, buf, MAXLINE);
        fputs(buf, stdout);
    }
    if(close(clientfd) < 0) {
        fprintf(stderr, "close clientfd error");
        exit(0);
    }
    exit(0);
}

int open_clientfd(char *hostname, char *port) {
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;
    
    // 获取可能的server地址
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; //tcp
    hints.ai_flags = AI_NUMERICSERV; // 使用数字端口号
    // 只有当主机配置了IPv4地址才进行查询IPv4地址；只有当主机配置了IPv6地址才进行查询IPv6地址.
    hints.ai_flags |= AI_ADDRCONFIG; 
    if((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }

    for(p = listp; p; p = p->ai_next) {
        if((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;   // 创建socket失败 尝试下一个地址
        if(connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break;  // 创建成功
        // 创建失败，关闭clientfd
        if(close(clientfd) < 0) {
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        }
    }
    freeaddrinfo(listp);
    // 所有都尝试失败
    if(!p)
        return -1;
    else
        return clientfd;
}