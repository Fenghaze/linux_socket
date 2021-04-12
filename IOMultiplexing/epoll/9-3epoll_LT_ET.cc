/**
  * @file    :9-3epoll_LT_ET.cc
  * @author  :zhl
  * @date    :2021-03-18
  * @desc    :epoll模型LT和ET的区别
  * socket读缓冲区接收对端发送的数据
  * buf[BUFFER_SIZE]为服务端每次从读缓冲区读取的数据，即服务端一次处理的字符个数为BUFFER_SIZE
  * LT触发模式：当发送的数据大于BUFFER_SIZE时，还有数据在socket读缓冲区中，
  *           因此红黑树节点中还存在就绪事件，epoll_wait()还会触发，直到数据读完
  * ET触发模式：epoll_wait()只会触发一次，因此如果只read/recv一次，
  *           socket读缓冲区的数据可能还没有读完，因此需要使用while循环来读完，
  *           否则剩下的数据只有在下次对端有新的数据发送过来时，才能再次触发epoll_wait()进行读取
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
#include <errno.h>
#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10
#define SERVERPORT "12345"

//创建epoll句柄，红黑树根节点
static int epfd = epoll_create(5);
//epoll事件结构体数组，用于保存就绪事件
struct epoll_event events[MAX_EVENT_NUMBER];

//将fd设置为非阻塞读
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//向epfd上添加结点
void addfd(int fd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    //设置为ET触发模式
    if (enable_et)
    {
        ev.events |= EPOLLET;
    }
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

void et(struct epoll_event *events, int events_size, int lfd)
{
    char buf[BUFFER_SIZE];
    //遍历events数组中的所有就绪事件
    for (size_t i = 0; i < events_size; i++)
    {
        int sockfd = events[i].data.fd;
        //如果监听套接字发生读事件，说明有新的客户连接
        if (sockfd == lfd)
        {
            struct sockaddr_in raddr;
            socklen_t raddr_len = sizeof(raddr);
            char ip[INET_ADDRSTRLEN];
            int cfd = accept(lfd, (struct sockaddr *)&raddr, &raddr_len);
            inet_ntop(AF_INET, &raddr.sin_addr, ip, INET_ADDRSTRLEN);
            printf("client %s:%d\n", ip, ntohs(raddr.sin_port));
            //将新的连接cfd添加到根节点
            addfd(cfd, true);
        }
        //如果其它节点读事件就绪，则读取数据
        else if (events[i].events & EPOLLIN)
        {
            //ET触发模式仅在读缓冲区满时触发一次，因此只打印一次
            printf("event trigger once\n");

            //读缓冲区满时，需要使用循环将所有数据读出
            while (1)
            {
                memset(buf, '\0', BUFFER_SIZE);
                int ret = read(sockfd, buf, BUFFER_SIZE - 1);
                if (ret < 0)
                {
                    /*对于非阻塞IO，下面的条件成立表示数据已经全部读取完毕。
                    此后，epoll就能再次触发sockfd上的EPOLLIN事件，以驱动下一次读操作*/
                    if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                    {
                        break;
                    }
                    else
                    {
                        perror("read()");
                        close(sockfd);
                        exit(1);
                    }
                }
                else if (ret == 0)
                {
                    close(sockfd);
                }
                else
                {
                    printf("get %d bytes of content:%s\n", ret, buf);
                }
            }
        }
        else
        {
            printf("something else happened\n");
        }
    }
}

void lt(struct epoll_event *evetns, int events_size, int lfd)
{
    //server每次从socket缓冲区中处理BUFFER_SIZE(10)个字符
    char buf[BUFFER_SIZE];  
    for (size_t i = 0; i < events_size; i++)
    {
        int sockfd = events[i].data.fd;
        if (sockfd == lfd)
        {
            struct sockaddr_in raddr;
            socklen_t raddr_len = sizeof(raddr);
            char ip[INET_ADDRSTRLEN];
            int cfd = accept(lfd, (struct sockaddr *)&raddr, &raddr_len);
            inet_ntop(AF_INET, &raddr.sin_addr, ip, INET_ADDRSTRLEN);
            printf("client %s:%d\n", ip, ntohs(raddr.sin_port));
            //将新的连接cfd添加到根节点，并设置为LT模式
            addfd(cfd, false);
        }
        else if (events[i].events & EPOLLIN)
        {
            //LT触发模式：只要读缓冲区还有未读出的数据，就打印这句话
            printf("event trigger once\n");
            memset(buf, '\0', BUFFER_SIZE);
            int ret = read(sockfd, buf, BUFFER_SIZE - 1);
            if (ret <= 0)
            {
                close(sockfd);
                continue;
            }
            printf("get %d bytes of content:%s\n", ret, buf);
        }
        else
        {
            printf("something else happened\n");
        }
    }
}

int main(int argc, char const *argv[])
{
    int lfd;
    struct sockaddr_in laddr;

    lfd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

    bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

    listen(lfd, 5);
    //向epfd上添加监听套接字
    addfd(lfd, true);
    while (1)
    {
        //epoll监听等待
        int ret = epoll_wait(epfd, events, MAX_EVENT_NUMBER, NULL);
        if (ret < 0)
        {
            perror("epoll_wait()");
            exit(1);
        }
        //LT触发模式
        lt(events, ret, lfd);
        //ET触发模式    
        //et(events, ret, lfd);
    }
    close(lfd);
    return 0;
}