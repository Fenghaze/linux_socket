/**
 * @author: fenghaze
 * @date: 2021/05/31 09:35
 * @desc: 单例模式的懒汉模式日志类的实现
 */

#ifndef LOG_H
#define LOG_H
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <stdarg.h>
#include "locker.h"
#include "block_queue.h"

class Log
{
public:
    //懒汉模式
    static Log &getInstance()
    {
        static Log m_instance;
        return m_instance;
    }
    //异步写的工作线程
    static void *flush_log_thread(void *arg)
    {
        Log::getInstance().async_write_log();
    }

    bool init(const char *file_name, bool close_log, int log_bufsize = 8192, int split_lines = 5000000, int max_queue_size = 0);

    void write_log(int level, const char *format, ...);

    void flush(void);

private:
    Log();
    virtual ~Log();
    //异步写日志
    void *async_write_log()
    {
        std::string single_log;
        //从阻塞队列中获取日志，写入文件
        while (m_log_queue->pop(single_log))
        {
            //写文件需要加锁
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    char dir_name[128];                   //路径名
    char log_name[128];                   //log文件名
    int m_split_lines;                    //日志最大行数
    int m_log_bufsize;                    //日志缓冲区大小
    long long m_count;                    //当前日志行数
    int m_today;                          //按天数进行分类，记录当前是哪一天
    FILE *m_fp;                           //打开log文件指针
    char *m_buf;                          //输出的内容
    BlockQueue<std::string> *m_log_queue; //阻塞队列
    bool m_is_async;                      //是否异步
    locker m_mutex;                       //互斥锁
    bool m_close_log;                     //关闭日志
};

#define LOG_DEBUG(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}

Log::Log() : m_count(0), m_is_async(false) {}

Log::~Log()
{
    if (m_fp)
    {
        fclose(m_fp);
    }
}

//异步需要设置阻塞队列的长度，同步不需要设置
bool Log::init(const char *file_name, bool close_log, int log_bufsize, int split_lines, int max_queue_size)
{
    if (max_queue_size > 0)
    {
        m_is_async = true;
        m_log_queue = new BlockQueue<std::string>(max_queue_size);
        pthread_t tid;
        pthread_create(&tid, nullptr, flush_log_thread, nullptr);
    }
    m_close_log = close_log;
    m_log_bufsize = log_bufsize;
    m_buf = new char[m_log_bufsize];
    memset(m_buf, '\0', m_log_bufsize);
    m_split_lines = split_lines;

    time_t t = time(nullptr);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    //strrchr:搜索file_name中最后一次出现 '/' 之后的子串
    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    if (!p)
    {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else
    {
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;

    m_fp = fopen(log_full_name, "a");
    if (!m_fp)
    {
        return false;
    }

    return true;
}

void Log::write_log(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warning]:");
        break;
    case 3:
        strcpy(s, "[error]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }
    //写入一行log，行数+1
    m_mutex.lock();
    m_count++;
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) //everyday log
    {

        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};

        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }
    m_mutex.unlock();

    va_list valst;
    va_start(valst, format);

    std::string log_str;
    m_mutex.lock();
    //写入的具体时间内容格式
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);

    int m = vsnprintf(m_buf + n, m_log_bufsize - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock();

    if (m_is_async && !m_log_queue->full())
    {
        m_log_queue->push(log_str);
    }
    else
    {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valst);
}

void Log::flush(void)
{
    m_mutex.lock();
    //强制刷新写入流缓冲区
    fflush(m_fp);
    m_mutex.unlock();
}

#endif // LOG_H