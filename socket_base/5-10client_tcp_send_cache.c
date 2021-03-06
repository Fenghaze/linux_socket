/**
  * @file    :5-10client_tcp_send_cache.c
  * @author  :zhl
  * @date    :2021-03-11
  * @desc    :设置socket选项的TCP发送缓冲区大小，与5-11配合使用
  */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define SERVERPORT "12345"
#define BUFFERSIZE 512

int main(int argc, char const *argv[])
{
    int lfd;
    struct sockaddr_in raddr;
    
    lfd = socket(AF_INET, SOCK_STREAM, 0);

    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "127.0.0.1", &raddr.sin_addr);

    // 设置socket选项
    int sendbuf = atoi(argv[1]);   //命令行设置发送缓冲区大小，默认为 2048 bytes
    int sendlen = sizeof(sendbuf);
    setsockopt(lfd, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));

    // 读取socket选项
    getsockopt(lfd, SOL_SOCKET, SO_SNDBUF, &sendbuf, (socklen_t *)& sendlen);
    printf("the tcp send buffer size after setting is %d\n", sendbuf);

    connect(lfd, (struct sockaddr *)&raddr, sizeof(raddr));
    
    char send_msg[BUFFERSIZE];
    memset(send_msg, 'a', BUFFERSIZE);
    int n = send(lfd, send_msg, BUFFERSIZE, 0);
    printf("initial bufsize is %d, real send msg size is %d\n", BUFFERSIZE, n);
    
    close(lfd);

    return 0;
}