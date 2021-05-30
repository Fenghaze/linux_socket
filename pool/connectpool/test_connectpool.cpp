/**
 * @author: fenghaze
 * @date: 2021/05/29 17:12
 * @desc: 测试连接池
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "connectpool.h"

int main(int argc, char const *argv[])
{
    ConnectionPool &connect_pool = ConnectionPool::getInstance();

    Statement *state;
    ResultSet *res;

    //从连接池中获得一个连接
    std::shared_ptr<Connection> con = connect_pool.getConnect();
    //获得一个数据库对象
    state = con->createStatement();
    //使用数据库
    state->execute("use test");
    //查询语句
    res = state->executeQuery("select * from UserInfo;");
    while (res->next())
    {
        int id = res->getInt("uid");
        std::string name = res->getString("username");
        std::string password = res->getString("password");
        std::cout << "id=" << id << "\tname=" << name << "\tpassword=" << password << std::endl;
    }
    std::cout << "current pool size:" << connect_pool.getSize() << std::endl;
    sleep(5);
    //归还连接
    connect_pool.retConnect(con);
    std::cout << "current pool size:" << connect_pool.getSize() << std::endl;
    // while (1)
    // {
    //     pause();
    // }

    return 0;
}