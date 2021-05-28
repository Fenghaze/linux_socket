/**
 * @author: fenghaze
 * @date: 2021/05/28 13:45
 * @desc: 
 */

#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <vector>
#include <iostream>
#include <pthread.h>
#include "mutex.h"

#define BUFFERSIZE 15
using namespace std;

class MemoryPool
{
public:
    //懒汉模式
    static MemoryPool &create()
    {
        static MemoryPool m_pInstance;
        return m_pInstance;
    }

    //加锁，获取一个内存块。如果内存池中没有足够的内存块，则会自动分配新的内存块
    //如果分配的内存块数目达到了最大值，则报错
    void *getMemory()
    {
        MyMutex m_lock(m_mutex); //调用这个函数时，自动加锁，函数运行完自动解锁

        if (m_blocks.empty())
        {
            if (m_maxAlloc == 0 || m_allocated < m_maxAlloc)
            {
                ++m_allocated;
                return new char[m_blocksize];
            }
            else
            {
                cout << "MemoryPool::getMemory exhausted." << endl;
                return (void *)nullptr;
            }
        }
        //取出最后一个内存块
        else
        {
            auto ptr = m_blocks.back();
            m_blocks.pop_back();
            return ptr;
        }
    }

    //加锁，释放当前内存块，将其归还内存池
    void releaseMemory(void *ptr)
    {
        MyMutex m_lock(m_mutex);
        m_blocks.push_back(reinterpret_cast<char *>(ptr));
    }

    //返回内存块大小
    inline size_t blockSize() const
    {
        return m_blocksize;
    }

    //返回内存池中内存块数目
    inline int allocated() const
    {
        return m_allocated;
    }

    //返回内存池中可用的内存块数目
    inline int available() const
    {
        return m_blocks.size();
    }

private:
    MemoryPool() {}

    //创建大小为blockSize的内存块，内存池预分配preAlloc个块
    MemoryPool(size_t blockSize, int preAlloc = 0, int maxAlloc = 0) : m_blocksize(blockSize), m_allocated(preAlloc), m_maxAlloc(maxAlloc)
    {
        if (preAlloc < 0 || maxAlloc < 0 || maxAlloc < preAlloc)
        {
            cout << "CMemPool::CMemPool parameter error." << endl;
            throw exception();
        }
        int reseve = BLOCK_RESERVE;
        if (preAlloc > reseve)
        {
            reseve = preAlloc;
        }
        if (maxAlloc > 0 && maxAlloc < reseve)
        {
            reseve = maxAlloc;
        }
        //扩充内存池
        m_blocks.reserve(reseve);
        //初始化内存块，每个内存块是一个char型数组
        for (int i = 0; i < preAlloc; i++)
        {
            m_blocks.push_back(new char[m_blocksize]);
        }
    }

    ~MemoryPool()
    {
        for (auto block : m_blocks)
        {
            delete[] block;
        }
        m_blocks.clear();
    }

    MemoryPool(const MemoryPool &);
    MemoryPool &operator=(const MemoryPool &);

private:
    enum
    {
        BLOCK_RESERVE = 32
    };
    size_t m_blocksize;      //每个内存块的大小
    int m_maxAlloc;          //最大内存块个数
    int m_allocated;         //当前分配的内存块
    vector<char *> m_blocks; //内存池存储结构，每个内存块是一个char型数组，用于存放数据
    Mutex m_mutex;           //互斥锁
};

#endif // MEMORYPOOL_H