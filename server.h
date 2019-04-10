#ifndef __SERVER_H__ 
#define __SERVER_H__

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<sys/wait.h>
#include<netdb.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include"rio.h"

#define	MAXLINE	 8192  /* Max text line length */
#define LISTENQ  1024  /* Second argument to listen() */

int open_listenfd(char* port);
int accept_connection(int listenfd);
void handle(int fd);
void read_request_headers(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void echo(int connfd);
int make_socket_non_blocking(int fd);
void unix_error(char *msg);

extern char **environ; /* Defined by libc */

#endif