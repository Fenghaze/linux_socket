/**
  * @file    :5-12getDaytimeServer.c
  * @author  :zhl
  * @date    :2021-03-12
  * @desc    :使用gethostbyname函数访问目标服务器上的daytime服务
  */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>

int main(int argc, char const *argv[])
{
    char *host = argv[1];   //目标主机地址
    // 获取host地址信息
    struct hostent *hostinfo = gethostbyname(host);
    // 获取daytime服务信息
    struct servent *servinfo = getservbyname("daytime", "tcp");
    printf("daytime port is %d\n", ntohs(servinfo->s_port));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = servinfo->s_port;
    addr.sin_addr = *(struct in_addr*)*hostinfo->h_addr_list;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    char buffer[128];
    int n = read(sockfd, buffer, sizeof(buffer));
    buffer[n] = '\0';
    printf("the day time is :%s\n", buffer);
    close(sockfd);
    return 0;
}