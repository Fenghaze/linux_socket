/**
 * @author: fenghaze
 * @date: 2021/05/22 16:00
 * @desc: 
 * Linux在内核中提供了对连接是否处于活动状态的定期检查机制，通过socket选项KEEPALIVE来激活它
 * 本程序实现类似于KEEPALIVE的机制：
 * 1、利用alarm函数周期性地触发SIGALRM信号
 * 2、SIGALRM信号处理函数：统一事件源，利用管道通知主循环执行定时器链表上的定时任务
 * 3、升序定时器链表的回调函数处理（重连、关闭）非活动连接
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <sys/epoll.h>
#include <signal.h>
#include "listClock.h"

#define SERVERPORT "7788"
#define BUFFERSIZE 10
#define MAXEVENTNUM 1024
#define FD_LIMIT 65535
#define TIMESLOT 5

static int epfd;
static int pipefd[2];
static ListClock list_clock;

int setnonblocking(int fd)
{
    int old = fcntl(fd, F_GETFL);
    int opt = old | O_NONBLOCK;
    fcntl(fd, F_SETFL, opt);
    return old;
}

void addfd(int epfd, int fd)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

//定时器的回调：关闭非活动连接
void clock_func(ClientData *user)
{
    //移除节点
    epoll_ctl(epfd, EPOLL_CTL_DEL, user->cfd, 0);
    assert(user);
    close(user->cfd);
    printf("close cfd %d\n", user->cfd);
}

//信号处理函数：使用管道pipefd[1]发送错误码给管道pipefd[0]
void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char *)msg, 1, 0);
    errno = save_errno;
}

//设置sig信号的信号处理函数
void addsig(int sig)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void time_handler()
{
    //调用tick，处理定时器任务
    list_clock.tick();
    //一次alarm调用只会引起一次SIGALRM信号
    //所以我们要重新定时，以不断触发SIGALRM信号
    alarm(TIMESLOT);
}

int main(int argc, char const *argv[])
{

    int lfd;
    struct sockaddr_in laddr;

    lfd = socket(AF_INET, SOL_SOCKET, 0);
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "127.0.0.1", &laddr.sin_addr);

    bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

    listen(lfd, 5);

    //epoll
    epfd = epoll_create(3);
    epoll_event events[MAXEVENTNUM];

    addfd(epfd, lfd);

    //创建双向管道
    socketpair(AF_INET, SOCK_STREAM, 0, pipefd);
    setnonblocking(pipefd[1]); //设置为非阻塞
    addfd(epfd, pipefd[0]);    //监听读事件

    //设置信号处理函数
    addsig(SIGALRM);
    addsig(SIGTERM);

    ClientData *users = new ClientData[FD_LIMIT];

    bool timeout = false;
    alarm(TIMESLOT); //TIMESLOT秒后触发一次SIGALRM信号

    bool stop_server = false;
    while (!stop_server)
    {
        int n = epoll_wait(epfd, events, MAXEVENTNUM, -1);
        if ((n < 0) && (errno != EINTR))
        {
            printf("epoll faliure\n");
            break;
        }
        for (size_t i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;
            //有新客户连接：接受连接；添加一个定时器
            if (fd == lfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int cfd = accept(lfd, (struct sockaddr *)&client_address, &client_addrlength);
                addfd(epfd, cfd);

                //初始化用户信息结构体
                users[cfd].addr = client_address;
                users[cfd].cfd = cfd;

                //定义一个定时器
                UtilTimer *timer = new UtilTimer;
                timer->callback = clock_func;
                timer->client_data = &users[cfd];
                time_t cur = time(nullptr);
                timer->expire = cur + 3 * TIMESLOT;

                //添加到定时器容器中
                list_clock.addClock(timer);
            }
            //信号管道pipefd[0]可读，说明有信号响应（信号是从pipefd[1]发送给pipefd[0]的）
            else if ((fd == pipefd[0]) && (events[i].events & EPOLLIN))
            {
                int sig;
                char signals[1024];
                int ret = recv(fd, signals, sizeof(signals), 0);
                if (ret == -1)
                {
                    continue;
                }
                else if (ret == 0)
                {
                    continue;
                }
                else
                {
                    for (size_t i = 0; i < ret; i++)
                    {
                        switch (i)
                        {
                        case SIGALRM: //定时信号触发
                        {
                            //用timeout变量标记有定时任务需要处理，但不立即处理定时任务
                            //这是因为定时任务的优先级不是很高，我们优先处理其他更重要的任务
                            timeout = true;
                            break;
                        }
                        case SIGTERM:
                        {
                            stop_server = true;
                        }
                        }
                    }
                }
            }

            //有客户数据
            else if (events[i].events & EPOLLIN)
            {
                printf("event trigger once\n");

                //读缓冲区满时，需要使用循环将所有数据读出
                while (1)
                {
                    memset(users[fd].buf, '\0', BUFFERSIZE);
                    int ret = read(fd, users[i].buf, BUFFERSIZE - 1);
                    if (ret < 0)
                    {
                        /*对于非阻塞IO，下面的条件成立表示数据已经全部读取完毕。
                    此后，epoll就能再次触发sockfd上的EPOLLIN事件，以驱动下一次读操作*/
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            break;
                        }
                        else    //发生读错误：关闭连接，移除定时器
                        {
                            clock_func(&users[fd]);
                            if (users[fd].timer)
                            {
                                list_clock.delClock(users[fd].timer);
                            }
                            break;
                        }
                    }
                    else if (ret == 0) //客户端关闭连接：关闭连接，移除定时器
                    {
                        clock_func(&users[fd]);
                        if (users[fd].timer)
                        {
                            list_clock.delClock(users[fd].timer);
                        }
                    }
                    else    //有数据可读：调整定时器顺序，以延迟关闭该连接的时间
                    {
                        if (users[fd].timer)
                        {
                            time_t cur = time(nullptr);
                            users[fd].timer->expire = cur + 3 * TIMESLOT;
                            printf("adjust timer once\n");
                            list_clock.adjustClock(users[fd].timer);
                        }
                        printf("get %d bytes of content:%s\n", ret, users[fd].buf);
                    }
                }
            }
        }
        /*最后处理定时事件，因为I/O事件有更高的优先级。
        当然，这样做将导致定时任务不能精确地按照预期的时间执行*/
        if (timeout)
        {
            time_handler();
            timeout = false;
        }
    }
    close(lfd);
    close(pipefd[1]);
    close(pipefd[0]);
    delete[] users;
    return 0;
}