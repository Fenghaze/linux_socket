/**
 * @author: fenghaze
 * @date: 2021/05/27 13:37
 * @desc: 基于链表、条件变量和互斥量实现生产者-消费者模型
 * - 一个生产者，多个消费者
 * - 使用线程来模拟消费者和生产者
 * - 利用链表作为存储数据的缓冲区，插入操作模拟产生者生产，删除操作来模拟消费者消费产品
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <list>

#define CONSUMERS 5

using namespace std;

//初始化互斥量、条件变量
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

list<int> lst; //数据缓冲区

//生产者
void *producer(void *arg)
{
    while (1)
    {
        int data = rand() % 1000;

        //生产数据，加锁list
        pthread_mutex_lock(&mutex);

        lst.push_back(data);
        printf("producer done, %d\n", data);

        //解锁
        pthread_mutex_unlock(&mutex);

        //唤醒等待的消费者
        pthread_cond_signal(&cond);
        sleep(1);
    }
    pthread_exit(nullptr);
}

//消费者
void *consumer(void *arg)
{
    int consumer_number = *(int *) arg;
    while (1)
    {
        //加锁
        pthread_mutex_lock(&mutex);

        while (lst.empty())
        {
            printf("no product, please wait ...\n");
            //解锁并等待唤醒
            pthread_cond_wait(&cond, &mutex);
        }
        printf("consumer_number=%d, %d\n",consumer_number, lst.front());
        lst.pop_front();

        //解锁
        pthread_mutex_unlock(&mutex);

        sleep(1);
    }
    pthread_exit(nullptr);
}

int main(int argc, char const *argv[])
{
    pthread_t t1;
    auto m_threads = new pthread_t[CONSUMERS];

    pthread_create(&t1, nullptr, producer, nullptr);
    for (int i = 0; i < CONSUMERS; i++)
    {
        printf("create the NO.%d consumer thread\n", i + 1);
        pthread_create(m_threads + i, nullptr, consumer, &i);
    }

    //线程等待
    pthread_join(t1, nullptr);
    for (int i = 0; i < CONSUMERS; i++)
    {
        pthread_join(*(m_threads + i), nullptr);
    }

    //清空缓冲区
    lst.clear();

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}
