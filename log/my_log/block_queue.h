/**
 * @author: fenghaze
 * @date: 2021/05/31 10:07
 * @desc: 阻塞队列-生产者消费者模型
 * 当队列为空时，从队列中获取元素的线程将会被挂起
 * 当队列是满时，往队列里添加元素的线程将会挂起
 * 线程安全，每个操作前都要先加互斥锁，操作完后，再解锁
 * C++ STL容器：多个线程读是线程安全的，多个线程写则需要加锁
 */

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H
#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <list>
#include "locker.h"

template <class T>
class BlockQueue
{

public:
    BlockQueue(int max_size = 1000):m_max_size(max_size), m_size(0)
    {
        if (max_size <= 0)
        {
            exit(-1);
        }
        //初始化list
        m_list = new std::list<T>(max_size);
    }

    ~BlockQueue()
    {
        m_mutex.lock();
        clear();
        delete m_list;
        m_mutex.unlock();
    }

    //向队列中添加元素（生产者），写操作，需要进行加锁
    bool push(const T &item)
    {
        m_mutex.lock();
        //如果队列满，则唤醒所有等待的消费者线程，来提取元素
        if (full())
        {
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        m_list->push_back(item);
        m_size++;
        //添加成功后，唤醒所有等待的消费者线程
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }
    //从队列中删除一个元素（消费者），写操作，需要进行加锁
    bool pop(T &item)
    {
        m_mutex.lock();
        //队列为空，则需要等待队列有数据，使用条件变量等待被唤醒
        while (empty())
        {
            //如果队列不可用，解锁
            if(!m_cond.wait(m_mutex.get()))
            {
                m_mutex.unlock();
                return false;
            }
        }
        m_list->pop_back();
        m_size--;
        m_mutex.unlock();
        return true;
    }
    //清空队列
    void clear()
    {
        m_list->clear();
        m_size = 0;
    }
    //队列是否为空
    bool empty()
    {
        return m_list->empty();
    }
    //队列是否满
    bool full()
    {
        return m_size >= m_max_size ? true : false;
    }

    //获得队首元素
    bool front(T &val)
    {
        if(!empty())
        {
            val =  m_list->front();
            return true;
        }
        return false;
    }
    //获得队尾元素
    bool back(T &val)
    {
        if(!empty())
        {
            val =  m_list->back();
            return true;
        }
        return false;
    }
    //获得当前队列的元素个数
    int size()
    {
        m_size =  m_list->size();
        return m_size;
    }
    int max_size()
    {
        return max_size;
    }

private:
    locker m_mutex;      //互斥锁，保护队列
    cond m_cond;         //条件变量，通知线程队列是否可用
    std::list<T> *m_list;//队列
    int m_size;          //队列大小
    int m_max_size;
};

#endif // BLOCK_QUEUE_H