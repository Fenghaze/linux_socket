#ifndef __THREADPOOL_H
#define __THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"

//线程池类，定义为模板类，模板参数T是任务类
template <typename T>
class threadpool
{

public:
    /*参数thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    /*往请求队列中添加任务*/
    bool append(T *request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *worker(void *arg);
    void run();

private:
    int m_thread_number;        //线程池中的线程数量
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //线程池
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //请求队列的互斥锁，保护请求队列
    sem m_queuestat;            //请求队列的信号量，判断请求队列是否有任务
    bool m_stop;                //是否结束线程
};

//定义构造函数：创建线程池，并分离线程
template <typename T>
threadpool<T>::threadpool(int thread_number, int max_requests)
    : m_thread_number(thread_number), m_max_requests(max_requests), m_stop(false), m_threads(nullptr)
{
    if ((thread_number <= 0) || (max_requests < 0))
    {
        throw std::exception();
    }
    //初始化线程池
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads)
    {
        throw std::exception();
    }
    //创建线程,并将它们都线程分离：之后不需要回收线程
    for (size_t i = 0; i < m_thread_number; i++)
    {
        printf("create the NO.%d thread\n", i+1);
        //创建线程
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        //线程分离
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

//析构：释放资源，并停止工作线程
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
    m_stop = true;
}

//往请求队列中添加任务：使用互斥锁保护请求队列
template <typename T>
bool threadpool<T>::append(T *request)
{
    //加锁
    m_queuelocker.lock();
    //判断请求队列的任务数量
    if (m_workqueue.size() > m_max_requests)
    {
        //解锁
        m_queuelocker.unlock();
        return false;
    }
    //添加任务
    m_workqueue.push_back(request);
    //解锁
    m_queuelocker.unlock();
    //增加信号量
    m_queuestat.post();
    return true;
}

//工作线程
template <typename T>
void *threadpool<T>::worker(void *arg)
{
    //强转为 threadpool* 类型
    threadpool *pool = (threadpool *)arg;
    //执行任务
    pool->run();
    return pool;
}

//工作线程任务：
template <typename T>
void threadpool<T>::run()
{
    while (!m_stop)
    {
        //等待信号量
        m_queuestat.wait();
        //加锁
        m_queuelocker.lock();
        if (m_workqueue.empty())
        {
            //解锁
            m_queuelocker.unlock();
            continue;
        }
        //获得任务
        T *request = m_workqueue.front();
        //出队
        m_workqueue.pop_front();
        //解锁
        m_queuelocker.unlock();
        if (!request)
        {
            continue;
        }
        //处理任务
        request->process();
    }
}

#endif
