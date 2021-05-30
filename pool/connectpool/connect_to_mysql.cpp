/**
 * @author: fenghaze
 * @date: 2021/05/29 12:32
 * @desc: 
 */

#include <stdlib.h>
#include <stdio.h>
#include <mysql/mysql.h>

int main(int argc, char const *argv[])
{
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char server[] = "localhost";
    char user[] = "root";
    char password[] = "12345";
    char database[] = "mysql";

    conn = mysql_init(nullptr);

    if (!mysql_real_connect(conn, server, user, password, database, 0, nullptr, 0))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    if (mysql_query(conn, "show tables"))
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        exit(1);
    }

    res = mysql_use_result(conn);
    printf("mysql tables in mysql database:\n");

    while ((row=mysql_fetch_row(res))!=nullptr)
    {
        printf("%s\n", row[0]);
    }

    mysql_free_result(res);
    mysql_close(conn);

    printf("finish\n");

    return 0;
}