/**
  * @file    :13-3sem.cc
  * @author  :zhl
  * @date    :2021-04-06
  * @desc    :使用IPC_PRIVATE信号量来实现父子进程同步
  */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/wait.h>

union semun
{
    int val;
    struct semid_ds *buf;
};

//信号量PV操作
void pv(int sem_id, int op)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = op;
    sem_b.sem_flg = SEM_UNDO;
    semop(sem_id, &sem_b, 1);
}

int main(int argc, char const *argv[])
{
    //创建信号量集，key=IPC_PRIVATE
    int sem_id = semget(IPC_PRIVATE, 1, 0666);
    union semun sem_un;
    sem_un.val = 1; //信号量

    //操作信号量
    semctl(sem_id, 0, SETVAL, sem_un);

    //创建子进程
    pid_t pid = fork();
    if (pid <0)
    {
        perror("fork()");
        exit(1);
    }
    else if (pid == 0)  //子进程执行的操作
    {
        printf("child try to get binary sem\n");
        //争取信号量
        pv(sem_id, -1);
        printf("child get the sem and would release it after 3 seconds\n");
        sleep(3);
        //释放信号量
        pv(sem_id, 1);
        exit(0);
    }
    else    //父进程也争取信号量
    {
        printf("parent try to get binary sem\n");
        //争取信号量
        pv(sem_id, -1);
        printf("parent get the sem and would release it after 3 seconds\n");
        sleep(3);
        //释放信号量
        pv(sem_id, 1);
    }
    
    //收尸子进程
    waitpid(pid, NULL, 0);
    //删除信号量集
    semctl(sem_id, 0, IPC_RMID, sem_un);
    return 0;
}