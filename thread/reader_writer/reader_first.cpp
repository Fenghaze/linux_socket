/**
 * @author: fenghaze
 * @date: 2021/05/27 16:40
 * @desc: 读者写者问题：读者优先
 * - 使用互斥锁对资源进行保护，实现互斥
 * - 使用信号量对资源进行访问，实现同步
 * - 读者优先是指：当有新的读者到来时，同时有写者在等待，则让新读者读
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define READERS 5    //读者数
#define WRITERS 30   //写者数
#define READSPEED 1  //读频率
#define WRITESPEED 1 //写频率

//写互斥锁
pthread_mutex_t mutex_write = PTHREAD_MUTEX_INITIALIZER;

//读资源信号量
sem_t sem_read;

//当前读者数量
int reader_num = 0;

//写的数据
int data = 0;

//写者，需要加锁，防止读者访问资源
void *writer(void *arg)
{
    int writer_id = *(int *)arg;
    while (1)
    {
        pthread_mutex_lock(&mutex_write);
        printf("writer %d join...\n", writer_id);
        int rd = rand()%100;
        printf("write data:%d\n", rd);
        data = rd;
        printf("writer %d exit...\n", writer_id);
        pthread_mutex_unlock(&mutex_write);
        sleep(WRITESPEED);
    }
}

void *reader(void *arg)
{
    int reader_id = *(int *)arg;
    while (1)
    {
        //等待资源
        sem_wait(&sem_read);
        reader_num++;   //读者数+1
        //有读者，不能写，加锁
        if (reader_num >= 1)
        {
            pthread_mutex_lock(&mutex_write);
        }
        //资源可读
        sem_post(&sem_read);
        
        printf("reader %d join...\n", reader_id);
        printf("read data:%d\n", data);
        
        printf("reader %d exit...\n", reader_id);
        sem_wait(&sem_read);
        reader_num--;
        if (reader_num == 0)
        {
            pthread_mutex_unlock(&mutex_write);
        }
        sem_post(&sem_read);
        sleep(READSPEED);
    }
}

int main(int argc, char const *argv[])
{
    pthread_t writers_t[WRITERS], readers_t[READERS];
    sem_init(&sem_read, 0, 1);
    for (int i = 0; i < WRITERS; i++)
    {
        pthread_create(&writers_t[i], nullptr, writer, &i);
    }
    for (int i = 0; i < READERS; i++)
    {
        pthread_create(&readers_t[i], nullptr, reader, &i);
    }

    for (int i = 0; i < WRITERS; i++)
    {
        pthread_join(writers_t[i], nullptr);
    }
    for (int i = 0; i < READERS; i++)
    {
        pthread_join(readers_t[i], nullptr);
    }

    return 0;
}