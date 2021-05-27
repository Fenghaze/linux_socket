/**
 * @author: fenghaze
 * @date: 2021/05/27 15:33
 * @desc: 基于环形队列、信号量和互斥量实现生产者-消费者模型
 * - 创建两个信号量：blank（空格）、data（数据）
 * - 生产者：关心空格的数量，空格的数量从n减少到0
 * - 消费者：关心数据的数量，数据的数量从0增加到n
 * 队列满足：
 * 1、消费者<=生产者
 * 2、消费者赶上生产者时，生产者先运行（队列空）
 * 3、生产者赶上消费者时，消费者先运行（队列满）
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define SIZE    10

sem_t sem_blank;
sem_t sem_data;

int ring[SIZE];

void *producer(void *arg)
{
    int i = 0;
    while (1)
    {
        //等待blank信号量，并存放数据
        sem_wait(&sem_blank);
        int data = rand() % 1000;
        ring[i] = data;
        i++;
        i %= SIZE;
        printf("producer done, data:%d\n", data);
        //增加data信号量
        sem_post(&sem_data);

        sleep(1);
    }
}

void *consumer(void *arg)
{
    int i = 0;
    while (1)
    {
        //等待data信号量，并取出数据
        sem_wait(&sem_data);
        int data = ring[i];
        i++;
        i %= SIZE;
        printf("consumer done, data:%d\n", data);
        //blank信号量增加
        sem_post(&sem_blank);

        usleep(1000);
    }
}

int main(int argc, char const *argv[])
{
    pthread_t t1, t2;
    sem_init(&sem_blank, 0, SIZE);
    sem_init(&sem_data, 0, 0);

    pthread_create(&t1, nullptr, producer, nullptr);
    pthread_create(&t2, nullptr, consumer, nullptr);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    sem_destroy(&sem_blank);
    sem_destroy(&sem_data);

    return 0;
}