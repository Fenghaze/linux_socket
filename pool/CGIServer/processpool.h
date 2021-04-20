/*
/* 15-1：半同步/半异步进程池的实现
debug:216,253,295
*/

#ifndef __PROCESSPOOL_H
#define __PROCESSPOOL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

//子进程类
class process
{
public:
    process() : m_pid(-1) {}

public:
    pid_t m_pid;      //子进程PID
    int m_pipedfd[2]; //用于和父进程通信的管道
};

//进程池类，定义为模板，其模板参数时处理逻辑任务的类
template <typename T>
class processpool
{
private:
    processpool(int lfd, int process_number = 8);
    processpool(const processpool &);
    processpool &operator=(const processpool &);

public:
    //单例模式，以保证程序最多创建一个processpool实例，这是程序正确处理信号的必要条件
    static processpool<T> *create(int lfd, int process_number = 8)
    {
        if (!m_instance)
        {
            m_instance = new processpool<T>(lfd, process_number);
        }
        return m_instance;
    }

    ~processpool()
    {
        delete[] m_sub_process;
    }
    //启动进程池
    void run();

private:
    void setup_sig_pipe();
    void run_parent();
    void run_child();

private:
    static const int MAX_PROCESS_NUMBER = 16;  /*进程池允许的最大子进程数量*/
    static const int USER_PER_PROCESS = 65536; /*每个子进程最多能处理的客户数量*/
    static const int MAX_EVENT_NUMBER = 10000; /*epoll最多能处理的事件数*/
    int m_process_number;                      /*进程池中的进程总数*/
    int m_idx;                                 /*子进程在池中的序号，从0开始*/
    int m_epollfd;                             /*每个进程都有一个epoll内核事件表，用m_epollfd标识*/
    int m_listenfd;                            /*监听socket*/
    int m_stop;                                /*子进程通过m_stop来决定是否停止运行*/
    process *m_sub_process;                    /*保存所有子进程的描述信息*/
    static processpool<T> *m_instance;         /*进程池静态实例，在类外初始化*/
};

//初始化静态变量
template <typename T>
processpool<T> *processpool<T>::m_instance = nullptr;

//全局信号管道
static int sig_pipefd[2];

//fd设置为非阻塞
static int setnonblocking(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

//epoll添加节点
static void addfd(int epfd, int fd)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

//删除节点
static void removefd(int epfd, int fd)
{

    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//回调函数
static void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//信号处理函数
static void addsig(int sig, void(handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定义构造函数
template <typename T>
processpool<T>::processpool(int lfd, int process_number)
    : m_listenfd(lfd), m_process_number(process_number), m_idx(-1), m_stop(false)
{
    assert((process_number > 0) && (process_number <= MAX_PROCESS_NUMBER));
    //创建进程池，包含 process_number 个子进程
    m_sub_process = new process[process_number];
    assert(m_sub_process);
    //创建 process_number 个子进程，并建立和父进程的管道
    for (size_t i = 0; i < process_number; i++)
    {
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipedfd);
        //assert(ret == 0);
        m_sub_process[i].m_pid = fork();
        assert(m_sub_process[i].m_pid >= 0);
        if (m_sub_process[i].m_pid == 0) //子进程
        {
            //关闭读端
            close(m_sub_process[i].m_pipedfd[0]);
            //更新子进程序号
            m_idx = i;
            break;
        }
        else //父进程
        {
            //关闭写端
            close(m_sub_process[i].m_pipedfd[1]);
            continue;
        }
    }
}

//统一事件源：使用epoll监听信号
template <typename T>
void processpool<T>::setup_sig_pipe()
{
    m_epollfd = epoll_create(1);
    assert(m_epollfd != -1);
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    //assert(ret != -1);
    //写端设置为非阻塞
    setnonblocking(sig_pipefd[1]);
    //监听读端
    addfd(m_epollfd, sig_pipefd[0]);
    //设置信号处理函数
    addsig(SIGCHLD, sig_handler);
    addsig(SIGTERM, sig_handler);
    addsig(SIGINT, sig_handler);
    addsig(SIGPIPE, SIG_IGN);
}

//运行
template <typename T>
void processpool<T>::run()
{
    if (m_idx != -1) //进程池中有子进程，则运行子进程代码
    {
        run_child();
        return;
    }
    run_parent(); //否则运行父进程代码
}

//运行子进程：每个子进程负责一个客户
template <typename T>
void processpool<T>::run_child()
{
    //统一事件源
    setup_sig_pipe();
    //父子进程通信的管道
    int pipefd = m_sub_process[m_idx].m_pipedfd[1];
    //监听管道
    addfd(m_epollfd, pipefd);

    epoll_event events[MAX_EVENT_NUMBER];
    //T是用户类，负责处理数据
    /*模板类T必须实现init方法，以初始化一个客户连接。我们直接使用connfd来索引逻辑处理对象（T类型的对象），以提高程序效率*/
    T *users = new T[USER_PER_PROCESS];
    assert(users);
    int ret = -1;
    while (!m_stop)
    {
        int n = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((n < 0) && (errno != EINTR))
        {
            perror("epoll_wait()");
            break;
        }
        for (size_t i = 0; i < n; i++)
        {
            int sockfd = events[i].data.fd;
            //管道可读：父进程发来消息，通知有新客户到来
            //接收新客户连接
            if ((sockfd == pipefd) && (events[i].events & EPOLLIN))
            {
                int client;
                ret = recv(sockfd, (char *)&client, sizeof(client), 0);
                if (((ret < 0) && (errno != EAGAIN)) || ret == 0)
                {
                    continue;
                }
                else
                {
                    struct sockaddr_in raddr;
                    socklen_t raddr_len = sizeof(raddr);
                    int cfd = accept(m_listenfd, (struct sockaddr *)&raddr, &raddr_len);
                    if (cfd < 0)
                    {
                        perror("accept()");
                        continue;
                    }
                    //监听cfd
                    addfd(m_epollfd, cfd);
                    //添加新客户
                    users[cfd].init(m_epollfd, cfd, raddr);
                }
            }
            //信号
            else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                int sig;
                char signals[1024];
                ret = recv(sockfd, signals, sizeof(signals), 0);
                if (ret <= 0)
                    continue;
                else
                {
                    for (size_t j = 0; j < ret; j++)
                    {
                        switch (signals[j])
                        {
                        case SIGCHLD:
                        {
                            pid_t pid;
                            int stat;
                            while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
                            {
                                continue;
                            }
                            break;
                        }
                        case SIGTERM:
                        case SIGINT:
                        {
                            m_stop = true;
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }
            }
            //有数据可读，用户类负责处理数据
            else if (events[i].events & EPOLLIN)
            {
                users[sockfd].process();
            }
            else
            {
                continue;
            }
        }
    }
    delete[] users;
    users = nullptr;
    close(pipefd);
    close(m_epollfd);
}

//运行父进程：监听lfd,signal
template <typename T>
void processpool<T>::run_parent()
{
    //统一事件源
    setup_sig_pipe();
    //监听lfd
    addfd(m_epollfd, m_listenfd);
    epoll_event events[MAX_EVENT_NUMBER];
    int sub_process_counter = 0;
    int new_conn = 1;
    int ret = -1;
    while (!m_stop)
    {
        int n = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((n < 0) && (errno != EINTR))
        {
            perror("epoll_wait()");
            break;
        }
        for (size_t i = 0; i < n; i++)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == m_listenfd) //有新客户，采用Round Robin方式分配子进程
            {
                int k = sub_process_counter;
                do
                {
                    if (m_sub_process[k].m_pid != -1)
                    {
                        break;
                    }
                    k = (k + 1) % sub_process_counter;
                } while (k != sub_process_counter);
                if (m_sub_process[i].m_pid == -1)
                {
                    m_stop = true;
                    break;
                }
                //分配子进程序号
                sub_process_counter = (k + 1) % m_process_number;
                //通知子进程有新的连接到来
                send(m_sub_process[i].m_pipedfd[0], (char *)&new_conn, sizeof(new_conn), 0);
                printf("send request to child %d\n", k);
            }
            //信号
            else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                int sig;
                char signals[1024];
                ret = recv(sockfd, signals, sizeof(signals), 0);
                if (ret <= 0)
                {
                    continue;
                }
                else
                {
                    for (size_t j = 0; j < ret; j++)
                    {
                        switch (signals[j])
                        {
                        case SIGCHLD: //子进程退出
                        {
                            pid_t pid;
                            int stat;
                            while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
                            {
                                for (size_t k = 0; k < m_process_number; k++)
                                {
                                    /*如果进程池中第i个子进程退出了，则主进程关闭相应的通信管道，并设置相应的m_pid为-1，以标记该子进程已经退出*/
                                    if (m_sub_process[k].m_pid == pid)
                                    {
                                        printf("child %d exit\n", k);
                                        close(m_sub_process[k].m_pipedfd[0]);
                                        m_sub_process[k].m_pid = -1;
                                    }
                                }
                            }
                            /*如果所有子进程都已经退出了，则父进程也退出*/
                            m_stop = false;
                            for (size_t k = 0; k < m_process_number; k++)
                            {
                                if (m_sub_process[k].m_pid != -1)
                                {
                                    m_stop = false;
                                }
                            }
                            break;
                        }
                        //父进程中断，杀死所有进程
                        case SIGTERM:
                        case SIGINT:
                        {
                            printf("kill all sub process now\n");
                            for (size_t k = 0; k < m_process_number; k++)
                            {
                                int pid = m_sub_process[k].m_pid;
                                if (pid != -1)
                                {
                                    kill(pid, SIGTERM);
                                }
                            }
                            break;
                        }
                        default:
                            break;
                        }
                    }
                }
            }
            else
            {
                continue;
            }
        }
    }
    close(m_epollfd);
}

#endif