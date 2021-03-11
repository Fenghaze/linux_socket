/**
  * @file    :5-6client_oob.c
  * @author  :zhl
  * @date    :2021-03-11
  * @desc    :客户端发送带外数据（OOB），与5-7配合使用
  */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define SERVERPORT  "12345"

int main(int argc, char const *argv[])
{
    int lfd;
    struct sockaddr_in raddr;
    socklen_t raddr_len;
    const char *oob_data = "abc";
    const char *data = "123";

    //创建本地socket
    lfd = socket(AF_INET, SOCK_STREAM, 0);

    //配置服务器socket地址
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "127.0.0.1", &raddr.sin_addr);

    //请求连接服务器
    connect(lfd, (struct sockaddr *)&raddr, sizeof(raddr));
    send(lfd, data, strlen(data), 0);
    send(lfd, oob_data, strlen(oob_data), MSG_OOB);
    send(lfd, data, strlen(data), 0);

    close(lfd);
    return 0;
}