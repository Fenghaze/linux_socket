/**
 * @author: fenghaze
 * @date: 2021/05/28 13:55
 * @desc: 封装互斥锁
 */

#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
//锁接口类
class Lock
{
public:
    virtual ~Lock() {}  //虚析构函数
    virtual void lock() const = 0;
    virtual void unlock() const = 0;
};

//互斥锁类
class Mutex : public Lock
{
public:
    Mutex()
    {
        pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    //重写虚函数
    virtual void lock() const
    {
        pthread_mutex_lock(&m_mutex);
    }
    virtual void unlock() const
    {
        pthread_mutex_unlock(&m_mutex);
    }

private:
    mutable pthread_mutex_t m_mutex;
};

//锁
class MyMutex
{
public:
    //初始化构造函数，进行自动加锁   
    MyMutex(const Lock &l):m_lock(l)
    {
        m_lock.lock();  //多态：父类引用指向子类实例，并调用虚函数
    }
    
    //消亡时，会自动调用析构函数，进行自动解锁   
    ~MyMutex()
    {
        m_lock.unlock();
    }

private:
    const Lock &m_lock;
};

#endif // MUTEX_H