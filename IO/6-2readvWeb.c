/**
  * @file    :6-2readvWeb.c
  * @author  :zhl
  * @date    :2021-03-12
  * @desc    :使用readv，writev函数在Web服务器上集中写
  * 当Web服务器解析完一个HTTP请求之后，如果目标文档存在且客户具有读取该文档的权限，那么它就需要发送一个HTTP应答来传输该文档。
  * 这个HTTP应答包含1个状态行、多个头部字段、1个空行和文档的内容。
  * 其中，前3部分的内容可能被Web服务器放置在一块内存中，而文档的内容则通常被读入到另外一块单独的内存中（通过read函数或mmap函数）。
  * 我们并不需要把这两部分内容拼接到一起再发送，而是可以使用writev函数将它们同时写出
  */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define SERVERPORT "12345"
/*定义两种HTTP状态码和状态信息*/
static const char*status_line[2]={"200 OK","500 Internal server error"};

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

    cfd = accept(lfd, (void *)&raddr, &raddr_len);

    //保存HTTP字段
    

    return 0;
}