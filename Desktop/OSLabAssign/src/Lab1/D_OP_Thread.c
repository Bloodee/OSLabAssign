#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

int temp = 0;

void *thread1(){
    for(int i =0; i<5000;i++)
            temp++;
    printf("Thread1 terminated, temp = %d\n",temp);
}

void *thread2(){
    for(int i=0; i<5000;i++)
            temp--;
    printf("Thread2 terminated, temp = %d\n",temp);
}

int main()
{
    pthread_t tid1,tid2;
    int ret1,ret2;
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
    printf("All termintaed, temp = %d\n",temp);

    return 0;
}