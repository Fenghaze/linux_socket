/**
 * @author: fenghaze
 * @date: 2021/05/29 14:50
 * @desc: 数据库连接池
 */

#ifndef CONNECTPOOL_H
#define CONNECTPOOL_H

#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <functional>

#include <mysql_driver.h>
#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

#include <exception>

#include <pthread.h>

using namespace sql;

//数据库连接池
class ConnectionPool
{

public:
    //懒汉模式，构造时进行初始化
    static ConnectionPool &getInstance(std::string name="root", std::string pwd="12345", std::string url="tcp://127.0.0.1:3306", int size=40)
    {
        static ConnectionPool m_pInstance(name, pwd, url, size);
        return m_pInstance;
    }
    //获得一个连接
    auto getConnect() -> std::shared_ptr<Connection>;
    //归还一个连接
    auto retConnect(std::shared_ptr<Connection> &conn) -> void;
    //获得连接池大小
    auto getSize() -> int;
    ~ConnectionPool();

private:
    //ConnectionPool() {}
    //构造函数
    ConnectionPool(std::string name, std::string pwd, std::string url, int size);
    //初始化连接池
    auto initPool(int initialSize) -> void;
    //毁坏连接池
    auto destoryPool() -> void;
    //销毁一个连接
    auto destoryOneConn() -> void;
    //扩大数据库连接池
    auto expandPool(int size) -> void;
    //缩小数据库连接池
    auto reducePool(int size) -> void;
    //添加一个连接
    auto addConn(int size) -> void;

private:
    std::string username;                            //账号
    std::string password;                            //密码
    std::string conn_url;                            //连接url
    int poolSize;                                    //连接池的大小
    pthread_mutex_t m_mutex;                         //保护池的互斥锁
    std::list<std::shared_ptr<Connection>> conn_lst; //存放所有连接
    Driver *driver;                                  //驱动
};

//获得一个连接
std::shared_ptr<Connection> ConnectionPool::getConnect()
{
    pthread_mutex_lock(&m_mutex);

    std::shared_ptr<Connection> sp = conn_lst.front();
    conn_lst.pop_front();

    pthread_mutex_unlock(&m_mutex);

    return sp;
}

//归还一个连接
void
ConnectionPool::retConnect(std::shared_ptr<Connection> &ret)
{
    pthread_mutex_lock(&m_mutex);

    conn_lst.push_back(ret);

    pthread_mutex_unlock(&m_mutex);
}

//获得连接池大小
int ConnectionPool::getSize()
{
    return conn_lst.size();
}

//析构函数
ConnectionPool::~ConnectionPool()
{
    destoryPool();
    std::cout << "close the connect pool" << std::endl;
}

//构造函数
ConnectionPool::ConnectionPool(std::string name, std::string pwd, std::string url, int size=40) : username(name), password(pwd), conn_url(url), poolSize(size)
{
    //获得mysql驱动
    driver = get_driver_instance();
    //初始化一半的空间
    initPool(poolSize / 2);
    //锁初始化
    pthread_mutex_init(&m_mutex, nullptr);
}

//初始化连接池
void ConnectionPool::initPool(int initialSize)
{
    //加锁
    pthread_mutex_lock(&m_mutex);

    addConn(initialSize);

    //解锁
    pthread_mutex_unlock(&m_mutex);
}
//毁坏连接池
void ConnectionPool::destoryPool()
{
    for (auto &conn : conn_lst)
    {
        //依次转移所有权，出作用域时，关闭连接，出作用域时智能指针自动释放
        //std::shared_ptr<Connection> &&sp = std::move(conn_lst.front());   //debug
        conn->close();
    }
}
//销毁一个连接
void ConnectionPool::destoryOneConn()
{
    //智能指针加std::move转移一个连接的“所有权”，当出作用域时，自动调用关闭connect
    //&&sp：右值引用
    std::shared_ptr<Connection> &&sp = std::move(conn_lst.front());
    //关闭连接
    sp->close();
    poolSize--;
}
//扩大数据库连接池
void ConnectionPool::expandPool(int size)
{
    //加锁
    pthread_mutex_lock(&m_mutex);

    addConn(size);

    //解锁
    pthread_mutex_unlock(&m_mutex);
}

//缩小数据库连接池
void ConnectionPool::reducePool(int size)
{
    //加锁
    pthread_mutex_lock(&m_mutex);

    if (size > poolSize)
    {
        return;
    }
    for (int i = 0; i < size; i++)
    {
        destoryOneConn();
    }
    poolSize -= size;
    //解锁
    pthread_mutex_unlock(&m_mutex);
}

//向池中添加连接
void ConnectionPool::addConn(int size)
{
    //建立size个连接
    for (int i = 0; i < size; i++)
    {
        Connection *conn = driver->connect(conn_url, username, password);
        std::shared_ptr<Connection> sp(conn, [](Connection *conn)
                                       { delete conn; });
        conn_lst.push_back(std::move(sp));
    }
}

#endif // CONNECTPOOL_H