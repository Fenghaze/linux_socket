/**
  * @file    :5-5client.c
  * @author  :zhl
  * @date    :2021-03-11
  * @desc    :客户端发起连接，与5-5server.c一起使用
  */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#define SERVERPORT "12345"

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
    if (connect(ld, (void *)&raddr, sizeof(raddr)) < 0)
    {
        perror("connect()");
        exit(1);
    }
    int n;
    while (1)
    {
        //n = read(ld, recv_msg, sizeof(recv_msg));
        n = recv(ld, recv_msg, sizeof(recv_msg), 0);
        if(n == 0)
        {
            printf("server close...\n");
            break;
        }
        if(n < 0)
        {
            perror("recv()");
            exit(1);
        }
        write(1, recv_msg, n);
    }
    close(ld);
    exit(0);
}