/**
  * @file    :6-3sendfile.cc
  * @author  :zhl
  * @date    :2021-03-13
  * @desc    :使用sendfile函数，从服务端发送一个文件给客户端、
  * sendfile使用两个文件描述符进行传输文件，在内核中完成，零拷贝，效率高
  * 程序接收一个参数，该参数指定传输的文件名
  */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <string.h>
#include <unistd.h>

#define SERVERPORT "12345"

int main(int argc, char const *argv[])
{
    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY);  //打开文件
    struct stat file_stat;
    fstat(fd, &file_stat);   //获取文件属性
    
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
        //使用sendfile在 连接套接字cfd 和 文件描述符fd 之间传输文件
        sendfile(cfd, fd, NULL, file_stat.st_size);
        close(cfd);
    }
    
    close(lfd);
    return 0;
}