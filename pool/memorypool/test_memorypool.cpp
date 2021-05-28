/**
 * @author: fenghaze
 * @date: 2021/05/28 15:20
 * @desc: 测试内存池
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include "memorypool.h"

#define BLOCKSIZE 1500
#define MAXBLOCKS 10

using namespace std;

int main(int argc, char const *argv[])
{
    MemoryPool &mempool = MemoryPool::create();
    cout << "mempool block size = " << mempool.blockSize() << endl;
    cout << "mempool allocated block num = " << mempool.allocated() << endl;
    cout << "mempool available block num = " << mempool.available() << endl;
    cout << endl;
    vector<void *> ptrs; //存放从内存池中获取的内存块
    for (int i = 0; i < MAXBLOCKS; i++)
    {
        ptrs.push_back(mempool.getMemory());
    }

    mempool.getMemory();

    //释放ptrs的内存块，归还给内存池
    for (auto it = ptrs.begin(); it != ptrs.end(); it++)
    {
        mempool.releaseMemory(*it);
        cout << "myPool1 available block num = " << mempool.available() << endl;
    }

    return 0;
}