/**
  * @file    :9-1.cc
  * @author  :zhl
  * @date    :2021-03-17
  * @desc    :使用select模型处理普通数据和带外数据
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

#define SERVERPORT "12345"

int main(int argc, char const *argv[])
{
    int lfd, cfd;
    struct sockaddr_in laddr, raddr;
    socklen_t raddr_len;
    //创建本地socket
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    //配置socket信息
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
    //绑定本地端口和本地lfd
    bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));
    //设置监听socket
    listen(lfd, 5);

    raddr_len = sizeof(raddr); 
    //生成连接socket
    cfd = accept(lfd, (struct sockaddr *)&raddr, &raddr_len);
    
    char buf[BUFSIZ];
    fd_set read_fds, exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);    //带外数据为异常fd
    //使用select处理普通数据和带外数据
    while (1)
    {
        memset(buf, '\0', BUFSIZ);
        FD_SET(cfd, &read_fds);
        FD_SET(cfd, &exception_fds);
        int ret = select(cfd+1, &read_fds, nullptr, &exception_fds, nullptr);
        if (ret < 0)
        {
            perror("select()");
            exit(1);
        }
        //如果可读集合中cfd为1，则接收到的是普通数据
        if (FD_ISSET(cfd, &read_fds))
        {
            ret = read(cfd, buf, sizeof(buf)-1);
            printf("get %d bytes of normal data:%s\n", ret, buf);
        }
        //带外数据为异常事件
        else if (FD_ISSET(cfd, &exception_fds))
        {
            //使用recv，MSG_OOB标志可以获取带外数据
            ret = recv(cfd, buf, sizeof(buf)-1, MSG_OOB);
            printf("get %d bytes of oob data:%s\n", ret, buf);
        }
    }
    close(cfd);
    close(lfd);

    return 0;
}