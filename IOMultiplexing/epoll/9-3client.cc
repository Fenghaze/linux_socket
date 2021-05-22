/**
  * @file    :client.cc
  * @author  :zhl
  * @date    :2021-03-11
  * @desc    :客户端发送TCP数据给服务端
  */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#define SERVERPORT "7788"

int main(int argc, char const *argv[])
{
    int ld;
    struct sockaddr_in raddr;
    socklen_t raddr_len;
    char recv_msg[BUFSIZ];
    // 1、创建本地套接字
    ld = socket(AF_INET, SOCK_STREAM, 0);
    if (ld < 0)
    {
        perror("socket()");
        exit(1);
    }

    // 配置服务端socket
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "127.0.0.1", &raddr.sin_addr);

    // 请求连接服务端
    if (connect(ld, (struct sockaddr *)&raddr, sizeof(raddr)) < 0)
    {
        perror("connect()");
        exit(1);
    }
    int n;
    char send_msg[10];
    while (1)
    {
        fgets(send_msg, sizeof(send_msg), stdin);
        //n = send(ld, send_msg, strlen(send_msg), 0);
        n = write(ld, send_msg, strlen(send_msg));
        printf("sent %d bytes of content:%s\n", n, send_msg);
    }
    close(ld);
    exit(0);
}