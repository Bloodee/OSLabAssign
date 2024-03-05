#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/syscall.h>

void *thread1(){
    pid_t pid = getpid();
    pid_t tid = syscall(SYS_gettid);
    printf("Thread1: tid = %d pid = %d \n",tid,pid);
    int ret = system("./thread");
    pthread_exit(NULL);
}

void *thread2(){
        pid_t pid=getpid();
        pid_t tid=syscall(SYS_gettid);
    printf("Thread2: tid = %d pid = %d \n",tid,pid);
    int ret = system("./thread");
    pthread_exit(NULL);
}

int main()
{
    pthread_t tid1,tid2;
    int ret1,ret2;
    ret1 = pthread_create(&tid1,NULL,thread1,NULL);
    if(ret1!=0){
        perror("pthread_create error.");
        exit(1);
    }
    ret2 = pthread_create(&tid2,NULL,thread2,NULL);
    if(ret2!=0){
        perror("pthread_create error");
        exit(1);
    }
    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);
    return 0;
}