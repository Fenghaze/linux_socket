/**
  * @file    :6-1dupCGI.c
  * @author  :zhl
  * @date    :2021-03-12
  * @desc    :使用dup函数实现CGI服务器
  */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>

#define SERVERPORT "12345"

int main(int argc, char const *argv[])
{
    const char *ip = "0.0.0.0";
    int lfd, cfd;
    struct sockaddr_in laddr, raddr;
    socklen_t raddr_len;

    lfd = socket(AF_INET, SOCK_STREAM, 0);

    int flag = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, ip, &laddr.sin_addr);

    bind(lfd, (void *)&laddr, sizeof(laddr));

    listen(lfd, 5);

    cfd = accept(lfd, (void *)&raddr, &raddr_len);
    fprintf(stdout, "hello world\n");
    close(STDOUT_FILENO);   //关闭文件输出流，目前最小的fd为1
    dup(cfd);      //复制文件描述符，cfd引用计数+1
    printf("abcd\n");       //stdout指向了cfd，任何写到stdout的数据都被重定向到cfd中
    close(cfd);

    close(lfd);
    return 0;
}