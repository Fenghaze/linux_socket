/**
  * @file    :http.cc
  * @author  :zhl
  * @date    :2021-04-15
  * @desc    :http.h的源文件
  */

#include "http.h"

/*定义HTTP响应的一些状态信息*/
const char *ok_200_title = "OK";

const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";

const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file from this server.\n";

const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";

const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requested file.\n";

/*网站的根目录*/
const char *doc_root = "/var/www/html";

int setnonblocking(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fctnl(fd, F_SETFL, new_opt);
    return old_opt;
}

void addfd(int epfd, int fd)
{
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

void removefd(int epfd, int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

//用户数
int HTTPConn::m_user_count = 0;
//epoll句柄
int HTTPConn::m_epollfd = -1;

//初始化客户连接：获得客户信息，并监听
void HTTPConn::init(int sockfd, const sockaddr_in &addr)
{
    m_sockfd = sockfd;
    m_address = addr;

    //开发环境下调试时方便端口复用，生产环境下应该禁用此选项
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    addfd(m_epollfd, m_sockfd);
    m_user_count++;
    //初始化其他成员变量
    init();
}

//关闭连接：删除节点，用户数-1
void HTTPConn::close_conn(bool real_close)
{
    if (real_close && (m_sockfd != -1))
    {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

//初始化成员变量：HTTP协议中的关键字段
void HTTPConn::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

//解析HTTP请求：主状态机
HTTP_CODE HTTPConn::process_request()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE res = NO_REQUEST;
    char *text = 0;
    while (((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) || ((line_status = parse_line()) == LINE_OK))
    {
        
    }
}

LINE_STATUS HTTPConn::parse_line()
{
}