/**
  * @file    :14-1dead_lock.cc
  * @author  :zhl
  * @date    :2021-04-13
  * @desc    :制造死锁情况：线程1拥有A锁，申请B锁；线程2拥有B锁，申请A锁
  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

int a = 0;
int b = 0;
pthread_mutex_t mutex_a;
pthread_mutex_t mutex_b;


void *thread1(void *arg)
{
    pthread_mutex_lock(&mutex_b);
    printf("thread 1 got mutex b, waiting for mutex a\n");
    b++;
    sleep(5);
    pthread_mutex_lock(&mutex_a);
    b += a;
    a++;
    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);
    
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{

    pthread_t tid;

    //初始化互斥锁
    pthread_mutex_init(&mutex_a, NULL);
    pthread_mutex_init(&mutex_b, NULL);

    //创建线程1
    pthread_create(&tid, NULL, thread1, NULL);

    //main线程加锁
    pthread_mutex_lock(&mutex_a);
    printf("main thread got mutex a, waiting for mutex b\n");
    sleep(5);
    a++;
    pthread_mutex_lock(&mutex_b);
    a += b;
    b++;
    //解锁
    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_a);

    //销毁资源
    pthread_join(tid, NULL);
    pthread_mutex_destroy(&mutex_a);
    pthread_mutex_destroy(&mutex_b);

    return 0;
}