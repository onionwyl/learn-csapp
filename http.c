#include"http.h"

void *handle(void *arg) {
    int fd = (int)arg;
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];// cgi文件名及cgi参数
    rio_t rio;

    rio_readinitb(&rio, fd);
    if(!rio_readlineb(&rio, buf, MAXLINE)) 
        return NULL;
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if(strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return NULL;
    }
    read_request_headers(&rio);

    // Parse uri
    is_static = parse_uri(uri, filename, cgiargs);
    // 判断cgi文件是否存在
    if(stat(filename, &sbuf) < 0){
        // handle error
        clienterror(fd, filename, "404", "Not found",
		    "Tiny couldn't find this file");
        return NULL;
    } 
    // 判断是否是访问静态网页
    if(is_static) {
        // 文件是否存在
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            // handle client error
            clienterror(fd, filename, "403", "Forbidden",
			    "Tiny couldn't read the file");
            return NULL;
        }
        serve_static(fd, filename, sbuf.st_size);
    } else {
        if(!S_ISREG(sbuf.st_mode) || !(S_IXUSR & sbuf.st_mode)) {
            // handle error
            clienterror(fd, filename, "403", "Forbidden",
			    "Tiny couldn't run the CGI program");
            return NULL;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
    return NULL;
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
            strcat(filename, "index.html");
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