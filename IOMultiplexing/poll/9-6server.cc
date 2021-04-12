/**
  * @file    :9-6server.cc
  * @author  :zhl
  * @date    :2021-03-25
  * @desc    :实现聊天室程序的服务端：
  *            1、接收新连接
  *            2、接收客户数据
  *            3、把客户数据发送给每一个连接到该服务器上的客户端
  */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#define SERVERPORT "12345"
#define BUFFER_SIZE 64
#define USER_LIMIT 5
#define FD_LIMIT 65535

//保存用户数据
struct client_data
{
    sockaddr_in addr;
    char *write_buf;
    char buf[BUFFER_SIZE];
};

//fd设置为非阻塞
int
setnonblocking(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

int main(int argc, char const *argv[])
{

    int lfd, cfd;
    struct sockaddr_in laddr, raddr;
    //创建clients数组，分配FD_LIMIT个对象，记录每个客户的数据，方便索引
    client_data *clients = new client_data[FD_LIMIT]; //有点类似哈希表，索引为cfd

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "127.0.0.1", &laddr.sin_addr);

    int val = 1;
    socklen_t val_len = sizeof(val);
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &val, val_len);

    bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

    listen(lfd, USER_LIMIT);

    //初始化poll
    pollfd fds[USER_LIMIT + 1];
    //监听lfd
    fds[0].fd = lfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;
    //初始化其他cfd
    for (size_t i = 1; i <= USER_LIMIT; i++)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
    }
    int users = 0; //记录请求的客户数量
    while (1)
    {
        int n = poll(fds, users + 1, -1); //nfds初始时只有一个lfd
        if(n < 0)
        {
            perror("poll()");
            break;
        }
        for (size_t i = 0; i < users + 1; i++)
        {
            int sockfd = fds[i].fd;
            if ((sockfd == lfd) && (fds[i].revents & POLLIN)) //有新客户连接
            {
                socklen_t raddr_len = sizeof(raddr);
                cfd = accept(sockfd, (struct sockaddr *)&raddr, &raddr_len);
                //优先判断users数量是否超过最大限制客户数量
                if (users >= USER_LIMIT)
                {
                    const char *err_msg = "too many users in the rooms, wait...\n";
                    printf("%s", err_msg);
                    send(cfd, err_msg, strlen(err_msg), 0);
                    close(cfd);
                    continue;
                }
                //客户+1
                users++;
                //添加到clients数组
                clients[cfd].addr = raddr;
                //将sockfd设置为非阻塞
                int opt = setnonblocking(cfd);
                //添加到poll中
                fds[users].fd = cfd;
                fds[users].events = POLLIN | POLLRDHUP | POLLERR;
                //打印客户信息
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &raddr.sin_addr, ip, INET_ADDRSTRLEN);
                printf("welcome client %s:%d, now have %d users\n", ip, ntohs(raddr.sin_port), users);
            }
            else if (fds[i].revents & POLLIN) //如果连接cfd有数据可读，则其他客户注册可写事件
            {
                //将客户数据保存到对应的client_data结构体中
                memset(clients[sockfd].buf, '\0', BUFFER_SIZE);
                int ret = read(sockfd, clients[sockfd].buf, BUFFER_SIZE - 1);
                if (ret < 0)
                {
                    if (errno != EAGAIN) //出错，clients数组、fds数组用户-1
                    {
                        close(sockfd);
                        //使用clients最后一个客户进行覆盖
                        clients[sockfd] = clients[fds[users].fd];
                        //fds也被最后一个fd覆盖
                        fds[i] = fds[users];
                        i--;     //游标上移一个位置
                        users--; //客户-1
                    }
                    else //==EAGAIN，说明读缓冲区读取完毕
                    {
                        continue;
                    }
                }
                else if (ret == 0) //客户主动关闭连接
                {
                    printf("No.%d client exit...\n", sockfd);
                    close(sockfd);
                    //使用clients最后一个客户进行覆盖
                    clients[sockfd] = clients[fds[users].fd];
                    //fds也被最后一个fd覆盖
                    fds[i] = fds[users];
                    i--;     //游标上移一个位置
                    users--; //客户-1
                }
                else //处理数据，设置其他cfd的可写事件
                {
                    printf("client No.%d: %s\n", sockfd, clients[sockfd].buf);
                    //为其他客户注册可写事件
                    for (size_t j = 1; j <= users; j++)
                    {
                        if (fds[j].fd == sockfd) //跳过当前cfd
                        {
                            continue;
                        }
                        //注销可读事件
                        fds[j].events |= ~POLLIN;
                        //注册可写事件
                        fds[j].events |= POLLOUT;
                        //写缓冲区为当前客户数据
                        clients[fds[j].fd].write_buf = clients[sockfd].buf;
                    }
                }
            }
            else if (fds[i].revents & POLLOUT) //如果cfd有数据可写，则发送数据到对端，并重新注册可读事件
            {
                if(!clients[sockfd].write_buf) continue;
                int ret = write(sockfd, clients[sockfd].write_buf, strlen(clients[sockfd].write_buf));
                clients[sockfd].write_buf = NULL;
                //注销可写事件
                fds[i].events |= ~POLLOUT;
                //重新注册可读事件
                fds[i].events |= POLLIN;
            }
            else if (fds[i].revents & POLLERR)
            {
                printf("get an error from %d\n", sockfd);
                int error = 0;
                socklen_t len = sizeof(errno);
                //通过getsockopt来获取fd上的错误码，并清除
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
                {
                    perror("getsockopt()");
                }
                continue;
            }
            else if (fds[i].revents & POLLRDHUP)
            {
                /*如果客户端关闭连接，则服务器也关闭对应的连接，并将用户总数减1*/
                close(sockfd);
                //使用clients最后一个客户进行覆盖
                clients[sockfd] = clients[fds[users].fd];
                //fds也被最后一个fd覆盖
                fds[i] = fds[users];
                i--;     //游标上移一个位置
                users--; //客户-1
                printf("a client left\n");
            }
        }
    }
    delete[] clients;
    close(lfd);
    return 0;
}