#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
 #include <arpa/inet.h>
#define SERV_PORT "1234"

int main()
{
    int cfd;
    struct sockaddr_in serv_addr;
    socklen_t len;
    char buf[BUFSIZ];
    int n;
    cfd = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(SERV_PORT));
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    if(connect(cfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        perror("connect()");
        exit(1);
    }
    
    while (1)
    {
        memset(buf, '\0', sizeof(buf));
        fgets(buf, sizeof(buf), stdin);
        // 向当前客户端的socket写入数据
        int write_len = write(cfd, buf, strlen(buf));
        printf("send content: %s(%d)\n", buf, write_len);
        // 服务端对写入的数据进行转化，将结果写入到客户端的socket
        // 再从客户端的socket读取数据
        int read_len = read(cfd, buf, sizeof(buf));
        write(STDOUT_FILENO, buf, read_len);
    }
    close(cfd);
    return 0;
}
