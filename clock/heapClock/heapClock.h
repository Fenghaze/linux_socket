/**
 * @author: fenghaze
 * @date: 2021/05/25 16:28
 * @desc: 实现时间堆
 */

#ifndef HEAPCLOCK_H
#define HEAPCLOCK_H
#include <time.h>
#include <netinet/in.h>
#include <vector>
using namespace std;

class HeapTimer;

class ClientData
{
    sockaddr_in addr;
    int cfd;
    HeapTimer *timer;
};

class HeapTimer
{
public:
    HeapTimer(int timeout)
    {
        expire = time(nullptr) + timeout;
    }

public:
    time_t expire; //定时器生效时间（绝对时间）
    void (*callback)(ClientData *);
    ClientData *user_data;
};

class HeapClock
{
public:
    //构造函数
    HeapClock(int cap) : capacity(cap), cur_size(0)
    {
        array = new HeapTimer *[capacity]; //创建堆数组
        if (!array)
        {
            throw exception();
        }
        for (int i = 0; i < capacity; i++)
        {
            array[i] = nullptr;
        }
    }
    //拷贝构造函数，用已有数组初始化
    HeapClock(HeapTimer **init_array, int size, int cap) : capacity(cap), cur_size(size)
    {
        if (capacity < size)
        {
            throw exception();
        }
        array = new HeapTimer *[capacity];
        if (!array)
        {
            throw exception();
        }
        for (int i = 0; i < capacity; i++)
        {
            array[i] = nullptr;
        }
        if (size != 0)
        {
            for (int i = 0; i < size; i++)
            {
                array[i] = init_array[i];
            }
            /*对数组中的第[(cur_size-1)/2]～0个元素执行下虑操作*/
            for (int i = (cur_size) / 2; i >= 0; i--)
            {
                percolate_down(i);
            }
        }
    }

    ~HeapClock()
    {
        for (int i = 0; i < cur_size; i++)
        {
            delete array[i];
        }
        delete[] array;
    }

public:
    //添加定时器
    void addClock(HeapTimer *timer)
    {
        if (!timer)
        {
            return;
        }
        //当前数组容量不够，则扩充一倍
        if (cur_size >= capacity)
        {
            resize();
        }
        //新插入一个元素，当前堆大小+1
        int hole = cur_size++;
        int parent = 0;

        /*对从空穴到根节点的路径上的所有节点执行上虑操作*/
        for (; hole > 0; hole = parent)
        {
            parent = (hole - 1) / 2;
            if (array[parent]->expire <= timer->expire)
            {
                break;
            }
            array[hole] = array[parent];
        }
        array[hole] = timer;
    }

    //删除定时器
    void delClock(HeapTimer *timer)
    {
        if (!timer)
        {
            return;
        }
        /*仅仅将目标定时器的回调函数设置为空，即所谓的延迟销毁。
        这将节省真正删除该定时器造成的开销，但这样做容易使堆数组膨胀*/
        timer->callback = nullptr;
    }

    //获得堆顶的定时器
    HeapTimer *top() const
    {
        if (empty())
        {
            return nullptr;
        }
        return array[0];
    }

    //删除堆顶的定时器
    void pop()
    {
        if (empty())
        {
            return;
        }
        if (array[0])
        {
            delete array[0];
            //调整堆
            array[0] = array[--cur_size];
            percolate_down(0);
        }
    }

    //心搏函数
    void tick()
    {
        auto tmp = array[0];
        time_t cur = time(nullptr);
        while (!empty())
        {
            if (!tmp)
            {
                break;
            }
            if (tmp->expire > cur)
            {
                break;
            }
            if (array[0]->callback)
            {
                array[0]->callback(array[0]->user_data);
            }
            pop();
            tmp = array[0];
        }
    }

    bool empty() const { return cur_size = 0; }

private:
    void percolate_down(int hole)
    {
        auto tmp = array[hole];
        int child = 0;
        for (; (hole * 2 + 1) <= cur_size - 1; hole=child)
        {
            child = hole * 2 + 1;
            if((child<cur_size-1) && (array[child+1]->expire < array[child]->expire))
            {
                child++;
            }
            if (array[child]->expire < tmp->expire)
            {
                array[hole] = array[child];
            }
            else
            {
                break;
            }
        }
        array[hole] = tmp;
    }

    void resize()
    {
        HeapTimer **tmp = new HeapTimer *[2 * capacity];
        for (int i = 0; i < 2*capacity; i++)
        {
            tmp[i] = nullptr;
        }
        if (!temp)
        {
            throw exception();
        }
        capacity = 2 * capacity;
        for (int i = 0; i < cur_size; i++)
        {
            temp[i] = array[i];
        }
        delete[] array;
        array = tmp;
    }

private:
    HeapTimer **array; //堆数组（二维）
    int capacity;      //数组容量
    int cur_size;      //数组当前包含的元素个数
};

#endif // HEAPCLOCK_H