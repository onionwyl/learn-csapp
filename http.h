#ifndef __HTTP_H__
#define __HTTP_H__

#include"utils.h"
#include"rio.h"

void *handle(void *fd);
void read_request_headers(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

extern char **environ; /* Defined by libc */

#endif