/**
  * @file    :6-5tee.cc
  * @author  :zhl
  * @date    :2021-03-15
  * @desc    :使用tee和splice实现tee程序，注意创建两个管道
  * tee程序功能：将数据输出到终端和另存到文件
  */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char const *argv[])
{
    if(argc!=2)
    {
        printf("usage: %s <file>\n", argv[0]);
        return 1;
    }
    int filefd = open(argv[1], O_CREAT|O_WRONLY|O_TRUNC, 0666);
    int pipefd[2];  
    int pipefd_file[2];
    pipe(pipefd);   
    pipe(pipefd_file);

    //使用splice函数读取标准输入内容，写到管道的写端中
    int ret = splice(STDIN_FILENO, nullptr, pipefd[1], nullptr, BUFSIZ, SPLICE_F_MORE|SPLICE_F_MOVE);

    //使用tee函数读取pipefd管道读端的内容，复制到pipefd_file的写端
    ret = tee(pipefd[0], pipefd_file[1], BUFSIZ, SPLICE_F_NONBLOCK);

    //使用splice函数读取pipefd管道读端中的内容，定向到标准输出中
    ret = splice(pipefd[0], nullptr, STDOUT_FILENO, nullptr, BUFSIZ, SPLICE_F_MORE|SPLICE_F_MOVE);
    
    //使用splice函数读取pipefd_file管道读端中的内容，定向到文件描述符中
    ret = splice(pipefd_file[0], nullptr, filefd, nullptr, BUFSIZ, SPLICE_F_MORE|SPLICE_F_MOVE);
    
    close(filefd);
    close(pipefd[0]);
    close(pipefd[1]);
    close(pipefd_file[0]);
    close(pipefd_file[1]);
    return 0;
}