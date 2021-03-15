/**
  * @file    :6-4splice.cc
  * @author  :zhl
  * @date    :2021-03-15
  * @desc    :使用splice在内核实现一个零拷贝的echo服务器
  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define SERVERPORT "12345"

int main(int argc, char const *argv[])
{
    int lfd, cfd;
    struct sockaddr_in laddr, raddr;
    socklen_t raddr_len;

    lfd = socket(AF_INET, SOCK_STREAM, 0);

    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

    bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

    listen(lfd, 5);

    raddr_len = sizeof(raddr);
    
    while (1)
    {
        cfd = accept(lfd, (struct sockaddr *)&raddr, &raddr_len);
        int pipefd[2];

        pipe(pipefd);   //创建管道。pipefd[0]是读端，从管道中读出数据；pipefd[1]是写端，写数据到管道中
        
        //客户端发送数据过来，cfd接收到数据，将cfd的数据写入到管道中
        int ret = splice(cfd, nullptr, pipefd[1], nullptr, BUFSIZ, SPLICE_F_MORE|SPLICE_F_MOVE);
        
        //管道在此的作用相当于存储媒介

        //读取管道中的数据，将数据移动到cfd，和服务端通信
        ret = splice(pipefd[0], nullptr, cfd, nullptr, BUFSIZ, SPLICE_F_MORE|SPLICE_F_MOVE);

        close(cfd);
    }
    close(lfd);
    return 0;
}