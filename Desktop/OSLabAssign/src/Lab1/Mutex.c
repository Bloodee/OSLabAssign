#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
typedef struct {
        int flag;
} spinlock_t;

int temp = 0;
void spinlock_init(spinlock_t *lock) {
        lock->flag = 0;
}

void spinlock_lock(spinlock_t *lock) {
 while (__sync_lock_test_and_set(&lock->flag, 1));
}

void spinlock_unlock(spinlock_t *lock) {
 __sync_lock_release(&lock->flag);
}

void *thread1(void *arg){
        spinlock_t *lock = (spinlock_t *)arg;
        for(int i =0; i<5000;i++){
        spinlock_lock(lock);
        temp++;
        spinlock_unlock(lock);
    }
    printf("Thread1 terminated, temp = %d\n",temp);
    pthread_exit(NULL);
}

void *thread2(void *arg){
        spinlock_t *lock = (spinlock_t *)arg;
        for(int i=0; i<5000;i++){
        spinlock_lock(lock);
        temp--;
        spinlock_unlock(lock);
    }
    printf("Thread2 terminated, temp = %d\n",temp);

    pthread_exit(NULL);
}

int main()
{

    pthread_t tid1,tid2;
    int ret1,ret2;
    spinlock_t lock;
    spinlock_init(&lock);
    ret1 = pthread_create(&tid1,NULL,thread1,&lock);
    if(ret1!=0){
        perror("pthread_create error.");
        exit(1);
    }
    ret2 = pthread_create(&tid2,NULL,thread2,&lock);
    if(ret2!=0){
        perror("pthread_create error");
        exit(1);
    }

    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);

    printf("All termintaed, temp = %d\n",temp);

    return 0;
}