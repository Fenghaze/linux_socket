/**
  * @file    :13-4client.cc
  * @author  :zhl
  * @date    :2021-04-12
  * @desc    :实现聊天室程序的客户端：
  *             1、从标准输入终端读取用户数据，并将其发送至服务端
  *             2、接收服务端发送的消息，并将其打印至标准输出终端
  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define BUFSER_SIZE 1024
#define SERVERPORT "12345"
#define MAX_EVENT_NUMS 65535

int setnonblocking(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

void addfd(int epoll_fd, int fd)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

int main(int argc, char const *argv[])
{
    int cfd;
    struct sockaddr_in raddr;

    cfd = socket(AF_INET, SOCK_STREAM, 0);

    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &raddr.sin_addr);

    //请求连接远端socket
    connect(cfd, (struct sockaddr *)&raddr, sizeof(raddr));

    epoll_event events[MAX_EVENT_NUMS];
    int epfd = epoll_create(1);
    //监听标准输入的可读事件
    addfd(epfd, 0);
    //监听连接cfd的可读事件
    addfd(epfd, cfd);

    char recv_buf[BUFSER_SIZE];
    int pipefd[2];
    //创建管道用于读写数据
    pipe(pipefd);

    while (1)
    {
        int n = epoll_wait(epfd, events, MAX_EVENT_NUMS, -1);
        for (size_t i = 0; i < n; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == 0)
            {
                //使用splice实现零拷贝
                //从输入终端（0）读取数据，写入到管道写端（pipefd[1]）
                int ret = splice(0, nullptr, pipefd[1], nullptr, BUFSIZ, SPLICE_F_MORE | SPLICE_F_MOVE);
                //从管道读端（pipefd[0]）读取数据，写入到连接socket中
                ret = splice(pipefd[0], nullptr, cfd, nullptr, BUFSIZ, SPLICE_F_MORE | SPLICE_F_MOVE);
            }
            else if (sockfd == cfd)
            {
                memset(recv_buf, '\0', BUFSER_SIZE);
                int len = read(sockfd, recv_buf, BUFSER_SIZE - 1);
                write(1, recv_buf, len);
            }
            else
            {
                printf("server close the connection\n");
                exit(1);
            }
        }
    }

    close(cfd);
    return 0;
}