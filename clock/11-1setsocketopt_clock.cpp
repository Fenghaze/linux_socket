/**
 * @author: fenghaze
 * @date: 2021/05/22 14:13
 * @desc: 
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

#define SERVERPORT  "7777"

//超时连接函数
int timeout_connect(int time)
{
    int ret = 0;
    struct sockaddr_in raddr;

    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &raddr.sin_addr);

    //setsocketopt
    struct timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    socklen_t len = sizeof(timeout);
    ret = setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);

    ret = connect(cfd, (struct sockaddr *)&raddr, sizeof(raddr));
    if(ret == -1)
    {
        /*超时对应的错误号是EINPROGRESS。下面这个条件如果成立，我们就可以处理定时任务了*/
        if (errno == EINPROGRESS)
        {
            printf("connecting timeout,process timeout logic\n");
            return-1;
        }
        printf("error occur when connecting to server\n");
        return-1;
    }
    return cfd;
}

int main(int argc, char const *argv[])
{

    int cfd = timeout_connect(10);
    send(cfd, "hello", 5, 0);
    if (cfd < 0)
    {
        return 1;
    }

    return 0;
}