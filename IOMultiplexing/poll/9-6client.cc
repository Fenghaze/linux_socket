/**
  * @file    :9-6client.cc
  * @author  :zhl
  * @date    :2021-03-25
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
#include <poll.h>

#define BUFSER_SIZE     64
#define SERVERPORT      "12345"

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

    //使用poll模型监听fd
    pollfd fds[2];
    //监听标准输入的可读事件
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    //连接cfd的可读事件
    fds[1].fd = cfd;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char recv_buf[BUFSER_SIZE];
    int pipefd[2];
    //创建管道用于读写数据
    pipe(pipefd);

    while (1)
    {
        int n = poll(fds, 2, -1);
        if(fds[0].revents & POLLIN) //输入终端有数据，则将数据输入到cfd
        {
            //使用splice实现零拷贝
            //从输入终端（0）读取数据，写入到管道写端（pipefd[1]）
            int ret = splice(0, nullptr, pipefd[1], nullptr, BUFSIZ, SPLICE_F_MORE|SPLICE_F_MOVE);
            //从管道读端（pipefd[0]）读取数据，写入到连接socket中
            ret = splice(pipefd[0], nullptr, cfd, nullptr, BUFSIZ, SPLICE_F_MORE|SPLICE_F_MOVE);
        }
        if (fds[1].revents & POLLRDHUP) 
        {
            printf("server close the connection\n");
            exit(1);
        }
        else if (fds[1].revents & POLLIN)   //cfd有数据可读，则打印到输出终端
        {
            memset(recv_buf, '\0', BUFSER_SIZE);
            int len = read(fds[1].fd, recv_buf, BUFSER_SIZE-1);
            write(1, recv_buf, len);
        }
    }
    
    close(cfd);
    return 0;
}