#include <stdio.h>
#include <signal.h>
#include <unistd.h>
//全局计数器变量
int Cnt=0;
//SIGALRM信号处理函数
void CbSigAlrm(int signo)
{
    //输出定时提示信息
    printf("   seconds: %d",++Cnt);
    printf("\r");
    //又一次启动定时器，实现1秒定时
    alarm(1);
}
int main()
{
    //注册SIGALRM信号的信号处理函数
    if(signal(SIGALRM,CbSigAlrm)==SIG_ERR)
    {
        perror("signal");
        return -1;
    }
    //关闭标准输出的行缓存模式
    setbuf(stdout,NULL);
    //启动定时器，1s后触发SIGALRM信号
    alarm(1);
    //进程进入无限循环，仅仅能手动终止
    while(1)
    {

    	//暂停，等待信号
//    	pause();
    }
    return 0;
}