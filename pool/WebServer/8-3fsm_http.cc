/**
  * @file    :8-3fsm_http.cc
  * @author  :zhl
  * @date    :2021-04-19
  * @desc    :使用主、从两个有限状态机（fsm）读取和解析HTTP请求
  * 状态机的使用方法：
  * 1、定义整个流程中可能出现的状态
  * 2、为每个状态规定一系列动作，使用switch驱动每个状态
  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#define SERVERPORT "12345"
#define BUFFER_SIZE 1024

/*主状态机的两种可能状态，即分析哪个部分*/
enum CHECK_STATE
{
    CHECK_STATE_REQUESTLINE = 0, //当前正在分析请求行
    CHECK_STATE_HEADER           //当前正在分析头部字段
};
/*从状态机的三种可能状态，即行的读取状态*/
enum LINE_STATUS
{
    LINE_OK = 0, //读取到一个完整的行
    LINE_BAD,    //行出错
    LINE_OPEN    //行数据尚且不完整
};
/*服务器处理HTTP请求的结果*/
enum HTTP_CODE
{
    NO_REQUEST,        //请求不完整，需要继续读取客户数据
    GET_REQUEST,       //表示获得了一个完整的客户请求
    BAD_REQUEST,       //表示客户请求有语法错误
    FORBIDDEN_REQUEST, //表示客户对资源没有足够的访问权限
    INTERNAL_ERROR,    //表示服务器内部错误
    CLOSED_CONNECTION  //表示客户端已经关闭连接了
};
/*为了简化问题，我们没有给客户端发送一个完整的HTTP应答报文，而只是根据服务器的处理结果发送如下成功或失败信息*/
static const char *szret[] = {"I get a correct result\n", "Something wrong\n"};

//分析HTTP的行状态
LINE_STATUS parse_line(char *buffer, int &checked_index, int &read_index)
{
    char temp;
    //分析每个字节
    for (; checked_index < read_index; checked_index++)
    {
        //获得当前要分析的字节
        temp = buffer[checked_index];
        //当前为回车符，说明可能读取了一个完整行
        if (temp == '\r')
        {
            if (checked_index + 1 == read_index)
            {
                return LINE_OPEN;
            }
            //下一个字符为换行符，则说明读取到了一个完整行
            else if (buffer[checked_index + 1] == '\n')
            {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
        }
        //如果为换行符，则也有可能读取了一个完整行
        else if (temp == '\n')
        {
            if ((checked_index > 1) && (buffer[checked_index + 1] == '\r'))
            {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

//解析HTTP请求的每行内容
HTTP_CODE parse_requestline(char *temp, CHECK_STATE &checkstate)
{
    char *url = strpbrk(temp, " ");
    /*如果请求行中没有空“\t”字符或“ ”字符，则HTTP请求必有问题*/
    if (!url)
    {
        return BAD_REQUEST;
    }
    *url++ = '\0';
    char *method = temp;
    if (strcasecmp(method, "GET") == 0) /*仅支持GET方法*/
    {
        printf("The request method is GET\n");
    }
    else
    {
        return BAD_REQUEST;
    }
    url += strspn(url, " ");
    char *version = strpbrk(url, " ");
    if (!version)
    {
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " ");
    /*仅支持HTTP/1.1*/
    if (strcasecmp(version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }
    /*检查URL是否合法*/
    if (strncasecmp(url, "http://", 7) == 0)
    {
        url += 7;
        url = strchr(url, '/');
    }
    if (!url || url[0] != '/')
    {
        return BAD_REQUEST;
    }
    printf("The request URL is:%s\n", url);
    /*HTTP请求行处理完毕，状态转移到头部字段的分析*/
    checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HTTP_CODE parse_headers(char *temp)
{
    //遇到一个空行，说明得到一个正确的HTTP请求
    if (temp[0] == '\0')
    {
        return GET_REQUEST;
    }
    //处理 HOST 头部字段
    else if (strncasecmp(temp, "Host:", 5) == 0)
    {
        temp += 5;
        temp += strspn(temp, " ");
        printf("the request host is: %s\n", temp);
    }
    //其他头部字段不处理
    else
    {
        printf("i cannot handle the header\n");
    }
    return NO_REQUEST;
}

//解析HTTP请求的所有内容
HTTP_CODE parse_content(char *buffer, int &checked_index, CHECK_STATE &checkstate, int &read_index, int &start_line)
{
    //设置从状态机的初始状态
    LINE_STATUS linestatus = LINE_OK;
    HTTP_CODE rescode = NO_REQUEST;
    while ((linestatus = parse_line(buffer, checked_index, read_index)) == LINE_OK)
    {
        //当前行的在buffer中的起始地址
        char *temp = buffer + start_line;
        //记录下一行的起始地址
        start_line = checked_index;
        //主状态机条件
        switch (checkstate)
        {
        //分析当前请求行
        case CHECK_STATE_REQUESTLINE:
        {
            rescode = parse_requestline(temp, checkstate);
            //错误请求
            if (rescode == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }
        //分析头部字段
        case CHECK_STATE_HEADER:
        {
            rescode = parse_headers(temp);
            if (rescode == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        }
        default:
        {
            return INTERNAL_ERROR;
        }
        }
    }
    //如果没有读取到一个完整的行，则还需要继续读取客户数据才能进行分析
    if (linestatus == LINE_OPEN)
    {
        return NO_REQUEST;
    }
    else
    {
        return BAD_REQUEST;
    }
}

int main(int argc, char const *argv[])
{
    int lfd;
    struct sockaddr_in laddr, raddr;
    socklen_t raddr_len = sizeof(raddr);

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

    int val = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    bind(lfd, (struct sockaddr *)&laddr, sizeof(laddr));

    listen(lfd, 5);

    int cfd = accept(lfd, (struct sockaddr *)&raddr, &raddr_len);
    if (cfd < 0)
    {
        perror("accept()");
        exit(1);
    }
    else
    {
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        int data_read = 0;
        int read_index = 0;    //当前读取了多少字节客户数据
        int checked_index = 0; //当前分析了多少字节客户数据
        int start_line = 0;    //在buffer中的起始行号
        //设置主状态机的初始状态为分析请求行
        CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
        while (1) //循环读取数据并进行分析
        {
            //缓冲区大小逐渐减小
            data_read = recv(cfd, buffer + read_index, BUFFER_SIZE - read_index, 0);
            if (data_read == -1)
            {
                printf("reading failed\n");
                break;
            }
            else if (data_read == 0)
            {
                printf("client has closed the connection\n");
                break;
            }
            //更新读取的字节数
            read_index += data_read;
            //分析读取到的字节
            HTTP_CODE res = parse_content(buffer, checked_index, checkstate, read_index, start_line);
            //还没有读取到一个完整的HTTP请求
            if (res == NO_REQUEST)
            {
                continue;
            }
            //分析并得到一个完整的、正确的HTTP的GET请求
            else if (res == GET_REQUEST)
            {
                //响应消息
                printf("%s\n", szret[0]);
                send(cfd, szret[0], strlen(szret[0]), 0);
                break;
            }
            else
            {
                printf("%s\n", szret[1]);
                send(cfd, szret[1], strlen(szret[1]), 0);
                break;
            }
        }
        close(cfd);
    }
    close(lfd);
    return 0;
}