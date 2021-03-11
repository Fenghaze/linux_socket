/**
  * @file    :5-5server.c
  * @author  :zhl
  * @date    :2021-03-11
  * @desc    :服务端接受连接,使用 telnet 0.0.0.0 12345 进行测试
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
    int ld, connfd;
    struct sockaddr_in laddr, raddr;
    socklen_t raddr_len;
    char ipstr[INET_ADDRSTRLEN];
    char send_msg[BUFSIZ];
    // 1、创建监听套接字
    ld = socket(AF_INET, SOCK_STREAM, 0);
    if (ld < 0)
    {
        perror("socket()");
        exit(1);
    }

    // 设置socket属性
    int val = 1;
    if (setsockopt(ld, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    }

    // 2、bind
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
    if (bind(ld, (void *)&laddr, sizeof(laddr)) < 0)
    {
        perror("bind()");
        exit(1);
    }

    // 3、监听模式
    if (listen(ld, 200) < 0)
    {
        perror("listen()");
        exit(1);
    }

    raddr_len = sizeof(raddr);
    while (1)
    {
        // 4、接受连接，创建连接套接字
        connfd = accept(ld, (void *)&raddr, &raddr_len);
        if (connfd < 0)
        {
            perror("accept()");
            exit(1);
        }
        inet_ntop(AF_INET, &raddr.sin_addr, ipstr, INET_ADDRSTRLEN);
        printf("Client:%s:%d\n", ipstr, ntohs(raddr.sin_port));
        fgets(send_msg, sizeof(send_msg), stdin);
        //write(connfd, send_msg, strlen(send_msg));
        send(connfd, send_msg, strlen(send_msg), 0);
        close(connfd);
    }
    // 6、关闭socket
    close(ld);
    exit(0);
}