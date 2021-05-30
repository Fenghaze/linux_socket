#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <iostream>

using namespace sql;
using namespace std;

#define DBHOST "tcp://127.0.0.1:3306"
#define USER "root"
#define PWD "12345"

int main(int argc, char const *argv[])
{
    Driver *driver;
    Connection *conn;
    driver = get_driver_instance();
    conn = driver->connect(DBHOST, USER, PWD);
    if (conn == NULL)
    {
        cout << "conn is null" << endl;
    }
    cout << "connect suceess" << endl;
    delete conn;
    driver = nullptr;
    conn = nullptr;
    return 0;
}
