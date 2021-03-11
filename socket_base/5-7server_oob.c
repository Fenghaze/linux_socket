/**
  * @file    :5-7server_oob.c
  * @author  :zhl
  * @date    :2021-03-11
  * @desc    :服务端接受带外数据（OOB），与5-6配合使用
  */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define SERVERPORT "12345"

int main(int argc, char const *argv[])
{
    int lfd, connfd;
    struct sockaddr_in laddr, raddr;
    socklen_t raddr_len = sizeof(raddr);
    char recv_msg[BUFSIZ];

    // 创建本地socket
    lfd = socket(AF_INET, SOCK_STREAM, 0);

    // 配置本地socket地址
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

    // 绑定本地socket
    bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

    // 将本地socket变为监听socket
    listen(lfd, 5);

    // 接受客户端连接，创建一个新的连接socket
    connfd = accept(lfd, (struct sockaddr *)&raddr, &raddr_len);

    // 接收数据
    memset(recv_msg, '\0', BUFSIZ); // 清零操作
    int ret = recv(connfd, recv_msg, sizeof(recv_msg), 0);
    printf("got %d bytes of normal data'%s'\n", ret, recv_msg);
    // 接收OOB数据
    memset(recv_msg, '\0', BUFSIZ); // 清零操作
    ret = recv(connfd, recv_msg, sizeof(recv_msg), MSG_OOB);
    printf("got %d bytes of oob data'%s'\n", ret, recv_msg);
    // 接收数据
    memset(recv_msg, '\0', BUFSIZ); // 清零操作
    ret = recv(connfd, recv_msg, sizeof(recv_msg), 0);
    printf("got %d bytes of normal data'%s'\n", ret, recv_msg);

    close(connfd);
    close(lfd);
    return 0;
}