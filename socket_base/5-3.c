/**
  * @file    :5-3.c
  * @author  :zhl
  * @date    :2021-03-11
  * @desc    :服务端，测试backlog参数对listen的影响
  */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
static int stop = 0;
static void handle_term(int sig)
{
  stop = 1;
}

int main(int argc, char const *argv[])
{
  signal(SIGTERM, handle_term);
  if(argc <= 3)
  {
    printf("usage: %s ip_address port_number backlog\n", basename(argv[0]));
    return 1;
  }  
  const char *ip = argv[1];
  int port = atoi(argv[2]);
  int backlog = atoi(argv[3]);
  struct sockaddr_in addr;

  // 创建socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  assert(sock >= 0);

  // 配置socket
  addr.sin_family = AF_INET;    //设置协议族
  addr.sin_port = htons(port);  //转换为网络字节序，s=short
  inet_pton(AF_INET, ip, &addr.sin_addr); //ip转换为网络字节序

  // 绑定socket
  int res = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  assert(res != -1);

  // 监听socket
  int ret = listen(sock, backlog);
  assert(ret != -1);

  while (!stop)
  {
    sleep(1);
  }
  
  close(sock);
  return 0;
}
