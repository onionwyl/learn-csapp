#include "rio.h"


// size_t是unsigned long ssize_t 是 long
// read函数在出错时返回-1 所以大小缩小了一半
ssize_t rio_readn(int fd, void *usrbuf, size_t n) {
    size_t nleft = n; // 还剩多少未读
    ssize_t nread;  // 读了多少字节
    char *bufp = usrbuf;    
    while (nleft > 0) {
        if((nread = read(fd, bufp, nleft)) < 0) {
            if(errno == EINTR) //系统中断 则重启
                nread = 0;
            else
                return -1;
        } else if(nread == 0)  // 读到EOF
            break;
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);   // 返回读了多少字节 >=0

}
ssize_t rio_writen(int fd, void *usrbuf, size_t n) {
    size_t nleft = n;   // 还有多少未写
    ssize_t nwritten;   // 写了多少
    char *bufp = usrbuf;
    while(nleft > 0) {
        // write出错为-1，不过这里如果是0还不太明白，也许是写不进去了？
        if((nwritten = write(fd, bufp, nleft)) <= 0) {
            if(errno == EINTR)  // 中断
                nwritten == 0;
            else
                return -1;
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;   // 这里应该一定会都写出去，否则就是出错了
}
// 初始化rio数据结构
void rio_readinitb(rio_t *rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_cnt = 0;    // buffer中还有多少未读
    rp->rio_bufptr = rp->rio_buf;   // 指针指向buffer开头
}
/* 带缓冲的rio_read 对linux read()做了包装
 * 调用时，缓冲区有rio_cnt个字节未读 如果缓冲区为空，则会调用read填满它
 * 如果缓冲区非空，就会复制n和rio_cnt中更小的那个字节数给用户缓冲区，返回复制字节数
 */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
    int cnt;
    while(rp->rio_cnt <= 0) {   // 如果buf为空则读入
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if(rp->rio_cnt < 0) {
            if(errno != EINTR) // 同readn中用法
                return -1;
        } else if(rp->rio_cnt == 0) // 读到EOF
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;   // 因为重新读入，所以需要重置ptr

        // Copy
        cnt = n;
        if(rp->rio_cnt < n)
            cnt = rp->rio_cnt;
        memcpy(usrbuf, rp->rio_bufptr, cnt);
        rp->rio_bufptr += cnt;
        rp->rio_cnt -= cnt;
        return cnt;
    }
}
ssize_t	rio_readnb(rio_t *rp, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char* bufp = usrbuf;

    while(nleft > 0) {
        if((nread = rio_read(rp, bufp, nleft)) < 0) 
            return -1;  // read错误返回-1
        else if(nread == 0) // EOF
            break;
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);
}
ssize_t	rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    int n, rc;
    char c, *bufp = usrbuf;

    for(n = 1; n < maxlen; n++) {
        if((rc == rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if(c == '\n') {
                n++;
                break;
            }
        } else if(rc == 0) {
            if(n == 1)
                return 0;   // EOF 没读到任何数据
            else
                break;  // EOF 读了数据
        } else
            return -1;
    }
    *bufp = 0;  // 初始化指针，防止野指针么
    return n-1;
}