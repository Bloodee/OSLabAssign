#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<semaphore.h> 

int temp = 0;
sem_t signal; // 定义一个全局的信号量变量

void *thread1(){
    sem_wait(&signal); // P操作，申请一个资源
    for(int i =0; i<5000;i++)
            temp++;
    printf("Thread1 terminated, temp = %d\n",temp);
    sem_post(&signal); // V操作，释放一个资源
}

void *thread2(){
    sem_wait(&signal); // P操作，申请一个资源
    for(int i=0; i<5000;i++)
            temp--;
    printf("Thread2 terminated, temp = %d\n",temp);
    sem_post(&signal); // V操作，释放一个资源
}

int main()
{
    pthread_t tid1,tid2;
    int ret1,ret2;
    sem_init(&signal, 0, 1); // 初始化信号量的值为1
    ret1 = pthread_create(&tid1,NULL,thread1,NULL);
    if(ret1!=0){
        perror("pthread_create error.");
    }
    ret2 = pthread_create(&tid2,NULL,thread2,NULL);
    if(ret2!=0){
        perror("pthread_create error");
        exit(1);
    }

    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);
    sem_destroy(&signal); // 销毁信号量
    printf("All termintaed, temp = %d\n",temp);

    return 0;
}