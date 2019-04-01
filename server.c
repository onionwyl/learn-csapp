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
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];// cgi文件名及cgi参数
    rio_t rio;

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }
    read_request_headers(&rio);

    // Parse uri
    is_static = parse_uri(uri, filename, cgiargs);
    // 判断cgi文件是否存在
    if(stat(filename, &sbuf) < 0){
        // handle error
        clienterror(fd, filename, "404", "Not found",
		    "Tiny couldn't find this file");
        return;
    } 
    // 判断是否是访问静态网页
    if(is_static) {
        // 文件是否存在
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            // handle client error
            clienterror(fd, filename, "403", "Forbidden",
			    "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    } else {
        if(!S_ISREG(sbuf.st_mode) || !(S_IXUSR & sbuf.st_mode)) {
            // handle error
            clienterror(fd, filename, "403", "Forbidden",
			    "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}

// 读请求报头，以\r\n结尾
void read_request_headers(rio_t *rp) {
    char buf[MAXLINE];
    rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
        rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
}

// 处理uri，动态内容返回0，静态内容返回1
int parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;
    // 默认访问主页
    if(!strstr(uri, "cgi-bin")) {
        strcpy(cgiargs, "");    // 清空cgi变量
        strcpy(filename, ".");
        strcat(filename, uri);
        if(uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        return -1;
    } else {    // 动态内容
        ptr = index(uri, '?');
        if(ptr) {   // 将?后面的内容拷贝到cgiargs内，然后截断，并将剩余内容拷贝到filename中
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        } else 
            strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

void serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXLINE];

    // Response headers 写入
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); //HTTP版本，响应号
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf); //Server name
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    rio_writen(fd, buf, strlen(buf));   // header写入fd响应请求
    printf("Response headers:\n");
    printf("%s", buf);

    srcfd = open(filename, O_RDONLY, 0);
    // 使用mmap读取文件内容到内存
    srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    rio_writen(fd, srcp, filesize);
    munmap(srcp, filesize);
}

// 获取文件类型 用于填写response头部
void get_filetype(char *filename, char *filetype) {
    if(strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if(strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if(strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if(strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs) {
    char buf[MAXLINE], *emptylist[] = {NULL};
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    rio_writen(fd, buf, strlen(buf));
    if(fork() == 0) {
        printf("Query %s\n", cgiargs);
        setenv("QUERY_STRING", cgiargs, 1);
        if(dup2(fd, STDOUT_FILENO) < 0) {
            fprintf(stderr, "dup2 error\n");
            exit(1);
        }
        // 运行cgi程序
        execve(filename, emptylist, environ);
    }
    wait(NULL);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXLINE];
    // Response body
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    // Write response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, strlen(body));
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