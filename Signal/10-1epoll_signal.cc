/**
  * @file    :10-1epoll_signal.cc
  * @author  :zhl
  * @date    :2021-03-30
  * @desc    :统一事件源：让epoll模型同时监听信号事件
  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#define MAX_EVENT_NUMBER 1024
#define SERVERPORT "12345"

static int pipefd[2];              //信号读写管道
static int epfd = epoll_create(1); //epoll句柄

static int setnonblocking(int fd)
{
    int oldopt = fcntl(fd, F_GETFL);
    int newopt = oldopt | O_NONBLOCK;
    fcntl(fd, F_SETFL, newopt);
    return oldopt;
}

static void addfd(int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

//信号处理函数
static void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    //将信号写入管道，以通知主循环
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号及信号处理函数
static void addsig(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

int main(int argc, char const *argv[])
{
    int lfd;
    struct sockaddr_in laddr;

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "127.0.0.1", &laddr.sin_addr);

    bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

    listen(lfd, 5);

    //使用socketpair创建双向管道
    socketpair(AF_INET, SOCK_STREAM, 0, pipefd);
    setnonblocking(pipefd[1]); //设置为非阻塞

    //epfd事件组
    struct epoll_event events[MAX_EVENT_NUMBER];
    //添加lfd
    addfd(lfd);
    //添加pipefd[0],监听可读事件
    addfd(pipefd[0]);

    //设置需要监听的信号
    addsig(SIGHUP);     //挂起 ctrl+d
    addsig(SIGCHLD);    
    addsig(SIGTERM);    //终止
    addsig(SIGINT);     //中断 ctrl+c

    bool stop_server = false;
    while (!stop_server)
    {
        int n = epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
        if ((n < 0) && (errno != EINTR))
        {
            perror("epoll_wait()");
            break;
        }
        for (size_t i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;
            if (fd == lfd) //新客户连接
            {
                struct sockaddr_in raddr;
                socklen_t raddr_len = sizeof(raddr);
                int cfd = accept(fd, (struct sockaddr *)&raddr, &raddr_len);
                addfd(cfd);
            }
            //有信号
            else if ((fd == pipefd[0]) && (events[i].events & EPOLLIN))
            {
                int signum;
                char signal_msg[1024];
                int ret = read(fd, signal_msg, sizeof(signal_msg));
                if (ret == -1)
                    continue;
                else if (ret == 0)
                    continue;
                else
                {
                    /*因为每个信号值占1字节，所以按字节来逐个接收信号。
                    我们以SIGTERM为例，来说明如何安全地终止服务器主循环*/
                    for (size_t j = 0; j < ret; j++)
                    {
                        switch (signal_msg[j])
                        {
                            case SIGCHLD:
                            case SIGHUP:
                            {
                                continue;
                            }
                            case SIGTERM:
                            case SIGINT:
                            {
                                stop_server = true;
                            }
                        }
                    }
                }
            }
            else
            {
            }
        }
    }
    close(lfd);
    close(pipefd[1]);
    close(pipefd[0]);
    return 0;
}