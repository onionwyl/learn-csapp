#include"../server.h"

int main(void) {
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1=0, n2=0;

    /* Extract the two arguments */
    if((buf = getenv("QUERY_STRING")) != NULL && strlen(buf) != 0) {
        p = strchr(buf, '&');   // 找到第一次出现&的未知
        *p = '\0';
        strcpy(arg1, buf);  // 读取两个参数
        strcpy(arg2, p+1);
        n1 = atoi(arg1);    // 转换为数字
        n2 = atoi(arg2);
    }

    /* Make the response body */
    sprintf(content, "Welcome to add.com: ");
    sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
    sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", 
	    content, n1, n2, n1 + n2);
    sprintf(content, "%sThanks for visiting!\r\n", content);
  
    /* Generate the HTTP response */
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}