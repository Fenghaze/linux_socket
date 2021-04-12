/**
  * @file    :9-4epolloneshot.cc
  * @author  :zhl
  * @date    :2021-03-22
  * @desc    :为所有连接socket设置EPOLLONESHOT事件，使用线程来处理cfd
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
#include <pthread.h>
#include <errno.h>

#define MAX_EVENT_NUMBER    1024
#define BUFFER_SIZE         1024
#define SERVERPORT          "12345"

//epoll树根
static int epfd = epoll_create(1);

/*重置fd上的事件。这样操作之后，尽管fd上的EPOLLONESHOT事件被注册，
/*但是操作系统仍然会触发fd上的EPOLLIN事件，且只触发一次*/
void reset_oneshot(int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN|EPOLLET|EPOLLONESHOT;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

//添加节点
void addfd(int fd, bool oneshot)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN|EPOLLET;
    if (oneshot)    //设置为ONESHOT事件
    {
        ev.events |= EPOLLONESHOT;
    }
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

//处理数据的工作线程
static void *worker(void *arg)
{
    int sockfd = *(int *)arg;
    printf("start new thread to receive data on fd:%d\n", sockfd);
    //接收数据
    char buf[BUFFER_SIZE];
    memset(buf, '\0', BUFFER_SIZE);
    while (1)
    {
        int ret = read(sockfd, buf, BUFFER_SIZE-1);
        if(ret == 0)    //对端关闭连接
        {
            close(sockfd);
            printf("foreigner closed the connect...\n");
            break;
        }
        else if (ret<0)
        {
            if(errno == EAGAIN) //缓冲区读取完毕
            {
                reset_oneshot(sockfd);  //重置该socket上的注册事件，使得epoll可再次监听
                printf("read later\n");
                break;
            }
            else
            {
                printf("something error\n");
                break;
            }
        }
        else    //打印接收到的数据
        {
            write(1, buf, ret);
        }
    }
    printf("end thread receiving data on fd:%d\n", sockfd);
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    int lfd;
    struct sockaddr_in laddr;

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    //地址可重用
    int val = 1;
    int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    //设置socket地址
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

    bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

    //设置为监听socket
    listen(lfd, 5);

    //使用epoll模型监听等待
    struct epoll_event events[MAX_EVENT_NUMBER];
    addfd(lfd, true);
    while (1)
    {
        ret = epoll_wait(epfd, events, MAX_EVENT_NUMBER, NULL);
        if (ret < 0)
        {
            perror("epoll_wait()");
            exit(1);
        }
        //处理事件
        for (size_t i = 0; i < ret; i++)
        {
            int sockfd = events[i].data.fd;
            //如果lfd可读，说明有新客户连接
            if(sockfd == lfd)   
            {
                struct sockaddr_in raddr;
                socklen_t raddr_len = sizeof(raddr);
                int cfd = accept(sockfd, (struct sockaddr *)&raddr, &raddr_len);
                //将cfd添加到epfd
                addfd(cfd, true);
            }
            //其他cfd有数据传输
            else if (events[i].events&EPOLLIN)
            {
                pthread_t thread;
                //创建工作线程
                pthread_create(&thread, NULL, worker, &sockfd);
            }
            else
            {
                printf("something else happened\n");
            }
        }
    }
    close(lfd);
    return 0;
}