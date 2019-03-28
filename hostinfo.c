#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define	MAXLINE	 8192  /* Max text line length */
int main(int argc, char **argv)
{
    
    struct addrinfo *p, *listp, hints;
    char buf[MAXLINE];
    int rc, flags;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <domain name>\n", argv[0]);
        exit(0);
    }
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // 采用TCP连接
    // 主机名、服务名（端口号）、提示信息、返回的addrinfo指针
    // 函数目的是转换字符串地址为套接字地址
    if ((rc = getaddrinfo(argv[1], NULL, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
        exit(1);
    }


    flags = NI_NUMERICHOST;// 以数字形式而非名字返回主机地址
    for (p = listp; p; p = p->ai_next) {
        // 将套接字地址结构转换为主机+端口的字符串
        getnameinfo(p->ai_addr, p->ai_addrlen, buf, MAXLINE, NULL, 0, flags);
        printf("%s\n", buf);
    }
    freeaddrinfo(listp);
    exit(0);
}