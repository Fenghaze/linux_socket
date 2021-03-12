/**
  * @file    :5-11server_tcp_recv_cache.c
  * @author  :zhl
  * @date    :2021-03-11
  * @desc    :设置socket选项的TCP接收缓冲区大小
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
  int lfd, cfd;
  struct sockaddr_in laddr, raddr;
  socklen_t raddr_len;

  // 本地套接字
  lfd = socket(AF_INET, SOCK_STREAM, 0);

  // 配置socket地址
  laddr.sin_family = AF_INET;
  laddr.sin_port = htons(atoi(SERVERPORT));
  inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

  // 设置socket选项：TCP接收缓冲区大小
  int recvbuf = atoi(argv[1]);   //命令行设置接收缓冲区大小，默认为 256 bytes
  int recvlen = sizeof(recvbuf);
  setsockopt(lfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));
  // 获取socket选项
  getsockopt(lfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, (socklen_t *)&recvlen);
  printf("the tcp recv buffer size after setting is %d\n", recvbuf);

  // 绑定端口
  bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

  // 设置为监听套接字,被动监听
  listen(lfd, 5);

  raddr_len = sizeof(raddr);
  while (1)
  {
    cfd = accept(lfd, (struct sockaddr *)&raddr, &raddr_len);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &raddr.sin_addr, ip, INET_ADDRSTRLEN);
    printf("Client:%s:%d\n", ip, ntohs(raddr.sin_port));
    char recv_msg[BUFFERSIZE];
    memset(recv_msg, '\0', BUFFERSIZE);
    while(recv(cfd, recv_msg, BUFFERSIZE, 0)>0){}
    close(cfd);
  }

  close(lfd);
  return 0;
}