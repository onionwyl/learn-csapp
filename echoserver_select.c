#include"server.h"
#include<sys/select.h>
void command(void);

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    fd_set read_set, ready_set;

    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    listenfd = open_listenfd(argv[1]);
    FD_ZERO(&read_set);
    FD_SET(STDIN_FILENO, &read_set);
    FD_SET(listenfd, &read_set);

    while(1) {
        ready_set = read_set;
        select(listenfd+1, &ready_set, NULL, NULL,NULL);
        if(FD_ISSET(STDIN_FILENO, &ready_set)) {
            command();
        }
        if(FD_ISSET(listenfd, &ready_set)) {
            clientlen = sizeof(clientaddr);
            connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
            echo(connfd);
            close(connfd);
        }
    }

}

void command(void) {
    char buf[MAXLINE];
    if(!fgets(buf, MAXLINE, stdin)) 
        exit(0);
    printf("%s", buf);
}