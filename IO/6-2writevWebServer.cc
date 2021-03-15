/**
  * @file    :6-2readvWeb.c
  * @author  :zhl
  * @date    :2021-03-12
  * @desc    :使用readv，writev函数在Web服务器上集中写
  * 当Web服务器解析完一个HTTP请求之后，如果目标文档存在且客户具有读取该文档的权限，那么它就需要发送一个HTTP应答来传输该文档。
  * 这个HTTP应答包含1个状态行、多个头部字段、1个空行和文档的内容。
  * 其中，前3部分的内容可能被Web服务器放置在一块内存中，而文档的内容则通常被读入到另外一块单独的内存中（通过read函数或mmap函数）。
  * 我们并不需要把这两部分内容拼接到一起再发送，而是可以使用writev函数将它们同时写出
  * 
  * 程序运行效果：将目标文件作为第一个参数传递给网络服务器，客户telent到该服务端上获得该文件
  */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/uio.h>

#define BUFFER_SIZE 1024
#define SERVERPORT "12345"
/*定义两种HTTP状态码和状态信息*/
static const char *status_line[2] = {"200 OK", "500 Internal server error"};

int main(int argc, char const *argv[])
{
  int lfd, cfd;
  struct sockaddr_in laddr, raddr;
  socklen_t raddr_len;
  const char *file_name = argv[1]; //目标文件参数

  lfd = socket(AF_INET, SOCK_STREAM, 0);

  laddr.sin_family = AF_INET;
  laddr.sin_port = htons(atoi(SERVERPORT));
  inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

  bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

  listen(lfd, 5);

  raddr_len = sizeof(raddr);

  cfd = accept(lfd, (struct sockaddr *)&raddr, &raddr_len);

  //保存HTTP字段
  char header_buf[BUFFER_SIZE];
  int len = 0;
  memset(header_buf, '\0', BUFFER_SIZE);

  //存放目标文件内容的应用程序缓存
  char *file_buf;

  //获取目标文件的属性，如目录、文件大小等
  struct stat file_stat;

  //记录是否为有效文件
  bool valid = true;

  if (stat(file_name, &file_stat) < 0) //目标文件不存在
  {
    valid = false;
  }
  else
  {
    if (S_ISDIR(file_stat.st_mode)) //目标文件是一个目录
    {
      valid = false;
    }
    else if (file_stat.st_mode & S_IROTH) //当前用户拥有读取目标文件的权限
    {
      //打开文件
      int fd = open(file_name, O_RDONLY);
      //动态分配缓存区，并指定其大小为目标文件大小加1
      file_buf = new char[file_stat.st_size + 1];
      memset(file_buf, '\0', file_stat.st_size + 1);
      //将目标文件读入缓存区
      if (read(fd, file_buf, file_stat.st_size) < 0)
      {
        valid = false;
      }
    }
    else
    {
      valid = false;
    }
  }

  //目标文档存在且有效，则发送HTTP应答
  if(valid)
  {
    int ret = snprintf(header_buf, BUFFER_SIZE-1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
    len += ret;
    ret = snprintf(header_buf+len, BUFFER_SIZE-1-len, "Content-Length:%d\n", file_stat.st_size);
    len += ret;
    ret = snprintf(header_buf+len, BUFFER_SIZE-1-len, "%s", "\r\n");

    //使用writev将header_buf和file_buf一起写出
    struct iovec iv[2];
    iv[0].iov_base = header_buf;
    iv[0].iov_len = strlen(header_buf);
    iv[1].iov_base = file_buf;
    iv[1].iov_len = file_stat.st_size;
    ret = writev(cfd, iv, 2);
  }  
  else
  {
    int ret = snprintf(header_buf, BUFFER_SIZE-1, "%s%s\r\n", "HTTP/1.1", status_line[1]);
    len += ret;
    ret = snprintf(header_buf+len, BUFFER_SIZE-1-len, "%s", "\r\n");
    send(cfd, header_buf, strlen(header_buf), 0);
  }
  close(cfd);
  delete []file_buf;
  close(lfd);

  return 0;
}