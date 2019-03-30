#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netdb.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#include "rio.h"

#define	MAXLINE	 8192  /* Max text line length */
#define LISTENQ  1024  /* Second argument to listen() */

int open_listenfd(char* port);
void handle(int fd);