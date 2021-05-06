/**
  * @file    :15-6WebServer.cc
  * @author  :zhl
  * @date    :2021-04-15
  * @desc    :
  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "threadpool.h"
#include "http.h"

#define SERVERPORT "8888"
#define MAX_FD 65535
#define MAX_EVENT_NUMBER 10000

//声明外部函数
extern void addfd(int epfd, int fd, bool one_shot);
extern void removefd(int epfd, int fd);

void addsig(int sig, void(handler)(int), bool restart = true)
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

void show_error(int cfd, const char *info)
{
  printf("%s\n", info);
  send(cfd, info, strlen(info), 0);
  close(cfd);
}

int main(int argc, char const *argv[])
{

  int lfd;
  struct sockaddr_in laddr;
  int ret;

  //创建用于HTTP服务的线程池
  threadpool<HTTPConn> *pool = nullptr;
  try
  {
    pool = new threadpool<HTTPConn>;
  }
  catch (const std::exception &e)
  {
    exit(1);
  }

  //预先为每个可能的客户连接分配一个 HTTPConn 对象
  auto users = new HTTPConn[MAX_FD];
  assert(users);

  //客户索引
  int user_count = 0;

  //忽略SIGPIPE信号
  addsig(SIGPIPE, SIG_IGN);

  //socket配置
  lfd = socket(AF_INET, SOCK_STREAM, 0);
  laddr.sin_family = AF_INET;
  laddr.sin_port = htons(atoi(SERVERPORT));
  inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

  struct linger tmp = {1, 0};
  setsockopt(lfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

  ret = bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));
  assert(ret >= 0);
  ret = listen(lfd, 5);
  assert(ret >= 0);

  //初始化epoll模型
  int epfd = epoll_create(1);
  assert(epfd != -1);
  epoll_event events[MAX_EVENT_NUMBER];
  //监听lfd
  addfd(epfd, lfd, false);

  //初始化HTTP服务类的m_epollfd
  HTTPConn::m_epollfd = epfd;

  while (1)
  {
    int n = epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
    if ((n < 0) && (errno != EINTR))
    {
      printf("epoll wait failure\n");
      exit(1);
    }
    for (size_t i = 0; i < n; i++)
    {
      int sockfd = events[i].data.fd;
      //有新客户连接，分配一个线程
      if (sockfd == lfd)
      {
        struct sockaddr_in raddr;
        socklen_t raddr_len = sizeof(raddr);
        int cfd = accept(sockfd, (struct sockaddr *)&raddr, &raddr_len);
        if (cfd < 0)
        {
          fprintf(stdout, "errno is :%d\n", errno);
          continue;
        }
        if (HTTPConn::m_user_count >= MAX_FD)
        {
          show_error(cfd, "Internal server busy");
          continue;
        }
        //初始化客户连接
        users[cfd].init(cfd, raddr);
      }
      //出错事件
      else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
      {
        //关闭客户连接
        users[sockfd].close_conn(true);
      }
      //可读事件
      else if (events[i].events & EPOLLIN)
      {
        /*根据读的结果，决定是将任务添加到线程池，还是关闭连接*/
        if (users[sockfd].Read())
        {
          pool->append(users + sockfd);
        }
        else
        {
          users[sockfd].close_conn(true);
        }
      }
      //可写事件
      else if (events[i].events & EPOLLOUT)
      {
        /*根据写的结果，决定是否关闭连接*/
        if (!users[sockfd].Write())
        {
          users[sockfd].close_conn(true);
        }
        else
        {
        }
      }
    }
  }
  close(epfd);
  close(lfd);
  delete[] users;
  delete pool;
  return 0;
}