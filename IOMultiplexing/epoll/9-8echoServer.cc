/**
  * @file    :9-8echoServer.cc
  * @author  :zhl
  * @date    :2021-03-27
  * @desc    :使用epoll实现echo服务器，能够同时处理一个端口上的TCP和UDP请求
  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <errno.h>

#define MAX_EVENT_NUMBER 1024
#define TCP_BUFFER_SIZE 512
#define UDP_BUFFER_SIZE 1024
#define SERVERPORT "12345"

//epoll句柄
const int epfd = epoll_create(1);

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int fd)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET; //ET触发模式
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd); //fd设置为非阻塞
}

int main(int argc, char const *argv[])
{
    int tcpfd, udpfd;
    struct sockaddr_in laddr;

    //创建tcpsocket
    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "127.0.0.1", &laddr.sin_addr);

    bind(tcpfd, (struct sockaddr *)&laddr, sizeof(laddr));

    listen(tcpfd, 5);

    //创建udpsocket，并绑定在laddr
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "127.0.0.1", &laddr.sin_addr);

    bind(udpfd, (struct sockaddr *)&laddr, sizeof(laddr));

    //初始化epfd
    struct epoll_event events[MAX_EVENT_NUMBER];
    //将tcpfd、udpfd添加到epfd
    addfd(tcpfd);
    addfd(udpfd);

    while (1)
    {
        int n = epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
        if (n < 0)
        {
            perror("epoll_wait()");
            break;
        }
        for (size_t i = 0; i < n; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == tcpfd)
            {
                struct sockaddr_in raddr;
                socklen_t raddr_len = sizeof(raddr);
                int connfd = accept(sockfd, (struct sockaddr *)&raddr, &raddr_len);
                addfd(connfd);
            }
            else if (sockfd == udpfd) //udp连接，echo
            {
                char buf[UDP_BUFFER_SIZE];
                struct sockaddr_in raddr;
                socklen_t raddr_len = sizeof(raddr);
                memset(buf, '\0', UDP_BUFFER_SIZE);
                int ret = recvfrom(sockfd, buf, UDP_BUFFER_SIZE - 1, 0, (struct sockaddr *)&raddr, &raddr_len);
                if (ret > 0)
                {
                    sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&raddr, raddr_len);
                }
            }
            else if (events[i].events & EPOLLIN) //tcp连接
            {
                char buf[TCP_BUFFER_SIZE];
                while (1)
                {
                    memset(buf, '\0', TCP_BUFFER_SIZE);
                    int ret = read(sockfd, buf, TCP_BUFFER_SIZE - 1);
                    if (ret < 0)
                    {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            break;
                        }
                        close(sockfd);
                        break;
                    }
                    else if (ret == 0)
                    {
                        printf("client close\n");
                        close(sockfd);
                    }
                    else
                    {
                        send(sockfd, buf, strlen(buf), 0);
                    }
                }
            }
            else
            {
                printf("something else happened\n");
            }
        }
    }
    close(tcpfd);
    return 0;
}