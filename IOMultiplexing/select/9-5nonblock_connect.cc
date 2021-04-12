/**
  * @file    :9-5nonblock_connect.cc
  * @author  :zhl
  * @date    :2021-03-24
  * @desc    :使用select实现客户端的非阻塞connect
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
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>

#define SERVERPORT "12345"

//文件设置为非阻塞
int set_nonblcok(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//非阻塞connect
int nonblock_connect(int time)
{
    int fd;
    struct sockaddr_in raddr;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    //设置服务端socket地址
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "127.0.0.1", &raddr.sin_addr);

    //将fd设置为非阻塞
    int oldopt = set_nonblcok(fd);

    //请求连接服务端
    int ret = connect(fd, (struct sockaddr *)&raddr, sizeof(raddr));
    if (ret == 0) //立即连接，则恢复fd属性，并返回用于通信的fd
    {
        printf("connect with server immediately\n");
        fcntl(fd, F_SETFL, oldopt);
        return fd;
    }
    else if (errno != EINPROGRESS) //如果连接没有立即建立，且不是报EINPROGRESS错误
    {
        printf("nonblock connect not support\n");
        return -1;
    }
    else //EINPROGRESS错误，用select来监听写事件后，在进行判断
    {
        fd_set write_sets;
        struct timeval timeout;
        //清空写事件集合
        FD_ZERO(&write_sets);
        //监听fd的写事件
        FD_SET(fd, &write_sets);
        timeout.tv_sec = time;
        timeout.tv_usec = 0;
        ret = select(fd + 1, NULL, &write_sets, NULL, &timeout);
        if (ret <= 0)
        {
            perror("select()");
            close(fd);
            return -1;
        }
        if (!FD_ISSET(fd, &write_sets)) //fd没有写事件发生
        {
            printf("no event on sockfd found\n");
            close(fd);
            return -1;
        }

        int error = 0;
        socklen_t len = sizeof(errno);
        //通过getsockopt来获取fd上的错误码，并清除
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
        {
            perror("getsockopt()");
            close(fd);
            return -1;
        }
        if (error != 0) //如果错误码不为0，表示连接出错
        {
            printf("connection failed after select with the error:%d\n", error);
            close(fd);
            return -1;
        }
        //错误码为0，表示连接成功
        printf("connection ready after select with the socket:%d\n", fd);
        //恢复fd属性
        fcntl(fd, oldopt);
        return fd;
    }
}

int main(int argc, char const *argv[])
{

    int cfd = nonblock_connect(10);
    if (cfd < 0)
    {
        printf("failed\n");
        return 1;
    }
    close(cfd);
    return 0;
}