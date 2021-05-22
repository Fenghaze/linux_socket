/**
 * @author: fenghaze
 * @date: 2021/05/22 14:42
 * @desc: 升序链表定时器容器
 * 添加定时器的时间复杂度是O(n)
 * 删除定时器的时间复杂度是O(1)
 * 执行定时任务的时间复杂度是O(1)
 */

#ifndef LISTCLOCK_H
#define LISTCLOCK_H

#include <time.h>
#define BUFFERSIZE 10

class UtilTimer;

//用户数据类型
class ClientData
{
public:
    struct sockaddr_in addr;
    int cfd;
    char buf[BUFFERSIZE];
    UtilTimer *timer;
};

//定时器类型（升序链表中的每一个元素）
class UtilTimer
{
public:
    UtilTimer() : prev(nullptr), next(nullptr) {}

public:
    time_t expire;                  //任务的超时时间，使用绝对时间
    void (*callback)(ClientData *); //回调函数处理的客户数据，由定时器的执行者传递给回调函数
    ClientData *client_data;
    UtilTimer *prev; //指向前一个定时器
    UtilTimer *next; //指向后一个定时器
};

//升序的双向链表定时器容器：初始化链表、销毁链表、插入节点、修改节点、删除节点
class ListClock
{
public:
    //初始化双向链表
    ListClock() : head(nullptr), tail(nullptr) {}

    //销毁链表：删除所有节点
    ~ListClock()
    {
        auto tmp = head;
        while (tmp)
        {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }

    //插入节点：头插法，向链表中添加一个定时器
    void addClock(UtilTimer *timer)
    {
        if (!timer)
        {
            return;
        }

        //头节点为空
        if (!head)
        {
            head = timer;
            return;
        }
        //头插法：升序链表
        if (timer->expire < head->expire)
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        //其他情况
        addClock(timer, head);
    }

    //修改指定节点：根据定时器的expire值，调整节点位置
    void adjustClock(UtilTimer *timer)
    {
        if (!timer)
        {
            return;
        }
        auto tmp = timer->next;
        //timer在末尾 或 timer < tmp，不用调整
        if (!tmp || timer->expire < tmp->expire)
        {
            return;
        }

        //timer是头节点，则重新插入
        if(timer == head)
        {
            head = head->next;
            head->prev = nullptr;
            timer->next = nullptr;
            addClock(timer, head);
        }
        //不是头节点，则删除该节点，并重新插入
        else
        {
            timer->prev->next = tmp;
            tmp->prev = timer->prev;
            addClock(timer, tmp);
        }
    }

    //删除指定节点
    void delClock(UtilTimer *timer)
    {
        if(!timer) 
        {
            return;
        }
        //链表中只有一个定时器(链表长度为1)
        if((timer==head) && (timer==tail))
        {
            delete timer;
            head = nullptr;
            tail = nullptr;
            return;
        }

        //链表中至少有两个定时器，且timer==head
        if (timer == head)
        {
            head = head->next;
            head->prev = nullptr;
            delete timer;
            return;
        }

        //链表中至少有两个定时器，且timer==tail
        if(timer == tail)
        {
            tail = tail->prev;
            tail->next = nullptr;
            delete timer;
            return;
        }

        //timer不是head、tail
        timer->next->prev = timer->prev;
        timer->prev->next = timer->next;
        delete timer;
    }

    //处理定时事件：每隔一段事件调用一次来处理定时事件
    void tick()
    {
        if (!head)
        {
            return;
        }
        printf("timer tick\n");
        time_t cur = time(nullptr); //当前时间
        UtilTimer *tmp = head;
        while (tmp)
        {
            if(cur < tmp->expire)   //还没有达到处理时间，break，等待下一次调用tick()
            {
                break;
            }
            
            //执行回调函数，处理客户数据
            tmp->callback(tmp->client_data);
            
            //处理完一个定时器就删掉
            head = tmp->next;
            if (head)
            {
                head->prev = nullptr;
            }
            delete tmp;
            tmp = head;
        }
    }

private:
    //表示将目标定时器timer添加到节点lst_head之后的部分链表中  
    void addClock(UtilTimer *timer, UtilTimer *lst_head)
    {
        auto tmp = lst_head->next;
        auto pre = lst_head;
        while (tmp)
        {
            if(timer->expire < tmp->expire)
            {
                pre->next = timer;
                timer->next = tmp;
                timer->prev = pre;
                tmp->prev = timer;
                break;
            }
            pre = tmp;
            tmp = tmp->next;
        }
        //如果tmp < timer，则插入到末尾
        if(!tmp)
        {
            pre->next = timer;
            timer->prev = pre;
            timer->next = nullptr;
            tail = timer;
        }
    }

private:
    UtilTimer *head;
    UtilTimer *tail;
};

#endif // LISTCLOCK_H