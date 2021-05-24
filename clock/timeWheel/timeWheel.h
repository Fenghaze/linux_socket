/**
 * @author: fenghaze
 * @date: 2021/05/24 19:11
 * @desc: 实现时间轮
 */

#ifndef TIMEWHEEL_H
#define TIMEWHEEL_H

#include <time.h>
#include <stdio.h>
#include <netinet/in.h>
#include <vector>
#define BUFFERSIZE 10

using namespace std;

class TimeWheelTimer;

//用户数据类型
class ClientData
{
public:
    struct sockaddr_in addr;
    int cfd;
    char buf[BUFFERSIZE];
    TimeWheelTimer *timer;
};

//时间轮的槽所指向的链表
class TimeWheelTimer
{
public:
    TimeWheelTimer(int rot, int ts) : rotation(rot), time_slot(ts), next(nullptr), prev(nullptr) {}

public:
    int rotation;  //定时器在时间轮中转多少圈后生效，相当于expire
    int time_slot; //定时器对应的槽
    void (*callback)(ClientData *);
    ClientData *client_data;
    TimeWheelTimer *next; //指向链表中的前一个定时器
    TimeWheelTimer *prev; //指向链表中的后一个定时器
};

//时间轮定时器容器：初始化、销毁、插入节点、删除节点、执行定时任务
class TimeWheel
{
public:
    //初始化
    TimeWheel() : cur_slot(0)
    {
        //初始化时间轮的所有槽的链表
        for (auto slot : slots)
        {
            slot = nullptr;
        }
    }

    //销毁
    ~TimeWheel()
    {
        slots.clear();
    }

    //根据timeout创建一个定时器，并插入到合适的槽中
    TimeWheelTimer *addClock(int timeout)
    {
        if (timeout < 0)
        {
            return nullptr;
        }
        int ticks = 0;
        //小于一个槽间隔，则向上取1
        if (timeout < SI)
        {
            ticks = 1;
        }
        //否则，计算需要多少个槽间隔
        else
        {
            ticks = timeout / SI;
        }
        //计算需要转多少圈
        int rot = ticks / N;
        //计算最后的槽编号
        int ts = (cur_slot + (ticks % N)) % N;

        //创建定时器，转动rot圈后，第ts个槽上
        auto timer = new TimeWheelTimer(rot, ts);
        //如果第ts个槽为空，则这个定时器作为头节点
        if (!slots[ts])
        {
            printf("add timer, rotation=%d, ts=%d, cur_slot=%d\n", rot, ts, cur_slot);
            slots[ts] = timer;
        }
        //否则，使用头插法插入
        else
        {
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;
    }

    //根据timeout删除定时器
    void delClock(TimeWheelTimer *timer)
    {
        if (!timer)
        {
            return;
        }
        int ts = timer->time_slot;
        //如果删除的定时器是头节点
        if (timer == slots[ts])
        {
            slots[ts] = slots[ts]->next;
            if (slots[ts])
            {
                slots[ts]->prev = nullptr;
            }
            delete timer;
        }
        else
        {
            timer->prev->next = timer->next;
            if (timer->next)
            {
                timer->next->prev = timer->prev;
            }
            delete timer;
        }
    }

    //执行当前槽的所有定时事件
    void tick()
    {
        //取得当前槽的头节点
        TimeWheelTimer *tmp = slots[cur_slot];
        printf("current slot is %d\n", cur_slot);

        //遍历链表中的所有定时器
        while (tmp)
        {
            printf("tick the timer once\n");
            /*如果定时器的rotation值大于0，则它在这一轮不起作用*/
            if (tmp->rotation > 0)
            {
                tmp->rotation--;
                tmp = tmp->next;
            }
            /*否则，说明定时器已经到期，于是执行定时任务，然后删除该定时器*/
            else
            {
                tmp->callback(tmp->client_data);
                //如果是头节点
                if (tmp == slots[cur_slot])
                {
                    printf("delete header in cur_slot\n");
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if (slots[cur_slot])
                    {
                        slots[cur_slot]->prev = NULL;
                    }
                    tmp = slots[cur_slot];
                }
                else
                {
                    tmp->prev->next = tmp->next;
                    if (tmp->next)
                    {
                        tmp->next->prev = tmp->prev;
                    }
                    TimeWheelTimer *tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        /*更新时间轮的当前槽，以反映时间轮的转动*/
        cur_slot = ++cur_slot % N;
    }

private:
    static const int N = 60;           //槽的总数
    static const int SI = 1;           //槽间隔为1s
    vector<TimeWheelTimer *> slots{N}; //时间轮，每个元素指向一个定时器链表，链表无序
    int cur_slot;                      //当前槽
};

#endif // TIMEWHEEL_H