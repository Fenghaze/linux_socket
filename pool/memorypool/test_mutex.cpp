/**
 * @author: fenghaze
 * @date: 2021/05/28 14:23
 * @desc: 测试mutex.h
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include "mutex.h"

using namespace std;

//创建一个互斥锁实例
Mutex g_mutex;

//线程
void *start(void *arg)
{
    char *msg = (char *)arg;

    //自动加锁，保护下面的打印语句（按顺序执行），线程结束前会调用析构函数自动解锁
    MyMutex mymutex(g_mutex);   //如果注释掉这行语句，两个线程就会随机打印语句

    for (int i = 0; i < 5; i++)
    {
        cout << msg << endl;
        sleep(1);
    }
    pthread_exit(nullptr);
}

int main(int argc, char const *argv[])
{
    pthread_t thread1, thread2;
    pthread_attr_t attr1, attr2;

    char *pMsg1 = "First print thread.";
    char *pMsg2 = "Second print thread.";

    pthread_attr_init(&attr1);
    pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_JOINABLE);

    //创建两个工作线程，分别打印不同的消息
    if (pthread_create(&thread1, &attr1, start, pMsg1) == -1)
    {
        cout << "Thread 1: create failed" << endl;
    }
    pthread_attr_init(&attr2);
    pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_JOINABLE);
    if (pthread_create(&thread2, &attr2, start, pMsg2) == -1)
    {
        cout << "Thread 2: create failed" << endl;
    }

    //等待线程结束
    pthread_join(thread1, nullptr);
    pthread_join(thread2, nullptr);

    //关闭线程，释放资源
    pthread_attr_destroy(&attr1);
    pthread_attr_destroy(&attr2);

    return 0;
}