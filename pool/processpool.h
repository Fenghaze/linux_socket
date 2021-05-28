/**
 * @author: fenghaze
 * @date: 2021/05/28 11:21
 * @desc: 单例模式创建静态进程池类
 * 属性：子进程的总数量、子进程的编号、每个子进程处理的任务相关的变量
 * 方法：
 * - 构造函数：初始化进程池，为每个子进程创建管道
 * - 执行父进程：epoll监听信号、监听lfd可读事件、监听管道可读事件（和子进程通信）
 * - 执行子进程：
 * 
 */

#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H


class 


#endif // PROCESSPOOL_H