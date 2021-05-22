/**
 * @author: fenghaze
 * @date: 2021/05/18 15:01
 * @desc: libevent实现的epoll reactor
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/epoll.h>

#define MAX_EVENTS 1024
#define BUFFERSIZE 5
#define SERVERPORT "1234"

struct myevent_s
{
    int fd;                                          //要监听的文件描述符
    int events;                                      //监听事件
    void *arg;                                       //指向自身的结构体指针
    void (*callback)(int fd, int events, void *arg); //函数指针
    int status;                                      //是否在监听:1->在epfd上(监听), 0->不在(不监听)
    char buf[BUFFERSIZE];                            //缓冲区大小
    int len;                                         //数据长度
    long last_active;                                //记录每次加入epfd的时间
} myevent_s;

int epfd;                                  //全局变量
struct myevent_s g_events[MAX_EVENTS + 1]; //监听变化的事件

void senddata(int fd, int events, void *arg);
void recvdata(int fd, int events, void *arg);

//初始化自定义的事件结构体
void setevent(struct myevent_s *myevent, int fd, void (*callback)(int fd, int events, void *arg), void *arg)
{
    myevent->fd = fd;
    myevent->callback = callback;
    myevent->arg = arg;
    myevent->events = 0;
    myevent->status = 0;
    if (myevent->len <= 0)
    {
        memset(myevent->buf, 0, sizeof(myevent->buf));
        myevent->len = 0;
    }
    myevent->last_active = time(NULL);
}

//添加事件，将ptr指向自定义的事件结构体
void addevent(int epfd, int event, struct myevent_s *myevent)
{
    epoll_event ev;
    int op = 0;
    ev.data.ptr = myevent;
    ev.events = myevent->events = event;
    if (myevent->status == 0)
    {
        op = EPOLL_CTL_ADD;
        myevent->status = 1;
    }
    epoll_ctl(epfd, op, myevent->fd, &ev);
}

//删除节点
void delevent(int epfd, struct myevent_s *myevent)
{
    epoll_event ev;
    if (myevent->status != 1) //如果fd没有添加到监听树上，就不用删除，直接返回
        return;
    ev.data.ptr = NULL;
    ev.events = myevent->events = 0;
    myevent->status = 0;
    epoll_ctl(epfd, EPOLL_CTL_DEL, myevent->fd, &ev);
}

//修改节点信息
void ctlevent(int epfd, int event, struct myevent_s *myevent)
{
    epoll_event ev;
    ev.data.ptr = myevent;
    ev.events = myevent->events = event;
    epoll_ctl(epfd, EPOLL_CTL_MOD, myevent->fd, &ev);
}

int setnonblocking(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

//回调函数：发送数据给客户端
void senddata(int fd, int events, void *arg)
{
    struct myevent_s *myevent = (struct myevent_s *)arg;
    int res = send(fd, myevent->buf, myevent->len, 0);
    if (res > 0)
    {
        printf("send[fd=%d], [%d]%s\n", fd, res, myevent->buf);
        setevent(myevent, fd, recvdata, myevent);   //将该fd的回调函数改为recvdata
        ctlevent(epfd, EPOLLIN, myevent); //修改节点，设为监听读事件
    }
    else
    {
        close(myevent->fd); //关闭连接
        printf("send[fd=%d] error %s\n", fd, strerror(errno));
        delevent(epfd, myevent);
    }
}

//回调函数：读取客户端发过来的数据
void recvdata(int fd, int events, void *arg)
{
    //强转数据类型
    struct myevent_s *myevent = (struct myevent_s *)arg;
    while (1)
    {
        int res = recv(fd, myevent->buf, BUFFERSIZE, 0);
        if (res < 0)
        {
            if (errno != EAGAIN)
            {
                close(fd);
                printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
                delevent(epfd, myevent);
                exit(-1);
            }
            else
            {
                continue;
            }
        }
        else if (res == 0) //客户端关闭连接
        {
            close(fd);
            printf("client [fd=%d], closed\n", fd);
            delevent(epfd, myevent);
            return;
        }
        else
        {
            myevent->len = res; //已读字符串长度
            printf("get %d bytes of content:%s\n", res, myevent->buf);
            myevent->buf[res] = '\0';
            //设置该fd对应的回调函数为senddata
            setevent(myevent, fd, senddata, myevent);
            //修改fd,监听其写事件
            ctlevent(epfd, EPOLLOUT, myevent);
            return;
        }
    }
}

//回调函数：当有文件描述符就绪, epoll返回, 调用该函数与客户端建立连接
void acceptconn(int fd, int events, void *arg)
{
    struct sockaddr_in raddr;
    socklen_t len = sizeof(raddr);
    int cfd, i;
    cfd = accept(fd, (struct sockaddr *)&raddr, &len);
    do
    {
        for (i = 0; i < MAX_EVENTS; i++)
        {
            if (g_events[i].status == 0)
            {
                break;
            }
        }
        if (i == MAX_EVENTS) //超出连接数上限
        {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
            break;
        }
        //cfd设置为非阻塞
        setnonblocking(cfd);
        //初始化自定义事件结构体
        setevent(&g_events[i], cfd, recvdata, &g_events[i]);
        //添加节点，监听读事件
        addevent(epfd, EPOLLIN | EPOLLET, &g_events[i]);
    } while (0);
    printf("new connect[%s:%d],[time:%ld],pos[%d]\n", inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port), g_events[i].last_active, i);
}

void initSocket(int epfd)
{
    struct sockaddr_in laddr;
    int lfd;

    lfd = socket(AF_INET, SOCK_STREAM, 0);

    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

    int val = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

    listen(lfd, 5);

    //初始化自定义事件结构体，设置回调函数为acceptconn
    //g_events最后一个位置用来保存lfd的相关信息
    setevent(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);
    //添加树节点
    addevent(epfd, EPOLLIN | EPOLLET, &g_events[MAX_EVENTS]);
}

int main(int argc, char const *argv[])
{
    epfd = epoll_create(1);
    epoll_event events[MAX_EVENTS + 1];

    //初始化监听套接字
    initSocket(epfd);

    printf("server is running at port %d\n", atoi(SERVERPORT));
    int trigger = 0;
    while (1)
    {
        int n = epoll_wait(epfd, events, MAX_EVENTS + 1, -1);
        if (n < 0)
        {
            perror("epoll_wait()");
            exit(-1);
        }

        for (size_t i = 0; i < n; i++)
        {
            auto ev = (struct myevent_s *)events[i].data.ptr;
            //读事件
            if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN))
            {
                printf("fd=%d, trigger = %d\n", ev->fd, ++trigger);
                ev->callback(ev->fd, events[i].events, ev->arg);
            }
            //写事件
            if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))
            {
                ev->callback(ev->fd, events[i].events, ev->arg);
            }
        }
    }

    return 0;
}