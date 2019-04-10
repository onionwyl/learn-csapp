#ifndef __UTILS_H__
#define __UTILS_H__

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
void echo(int connfd);
int make_socket_non_blocking(int fd);
void unix_error(char *msg);


#endif // !__UTILS_H__