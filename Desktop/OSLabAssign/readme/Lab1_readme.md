# 题目完成情况

## 进程相关编程实验

**编写并多次运行图中代码**

运行结果：(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/waitNn.jpg)

Fork()创建子进程并用pid作为fork()函数的返回值，若返回位负值则fork失败退出程序；

若返回为0，则进入子进程，此时pid1=getpid()得到的即为子程序的pid值;

若返回不为0，则进入父进程，同时pid返回的值为子进程的pid值，而父进程中的pid1=getpid()则返回的为父进程的pid值

**删去wait()函数**:

运行结果：(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/NwaitNn.jpg)

根据运行结果，父子进程的输出顺序与含wait函数时相反，这是因为wait()函数会阻塞父进程，知道它的子进程终止，父进程才可以终止。因此在含wait函数的运行结果里，子进程优先于父进程输出；删去wait函数后，结果恰好相反。

**问题1**

在printf函数打印末尾加入\n时，运行结果不同，当\n存在时无论是否加入wait()函数，两运行结果形式一致。

加入\n运行结果为:
(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/wait.png)
(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/nwait.png)

**父进程和子进程分别操作全局变量**:

代码:

```c
#include<sys/types.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
int Num=0;

int main()
{
pid_t pid,pid1;
        pid = fork();

        if(pid <0){
            fprintf(stderr, "Fork Failed\n");
            return 1;
        }
        else if(pid == 0){
            pid1 = getpid();
            for(int i=0;i<5;i++){
                printf("child: Num %d\n", Num++);
                printf("child: Num addr %p\n", &Num);
     }
 }
        else{
            pid1 = getpid();
            Num = 127;
            printf("parent: Num %d\n",Num);
            printf("parent: Num addr%p\n",&Num);
            wait(NULL);
        }

        return 0;
}
```


运行结果:(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/4.png)

如运行结果所示，在父进程和子进程中对于同一变量的写入时独立的，这说明父子进程是两个独立的进程，互不影响；两进程中对于同一变量的地址是相同的，而该地址是虚拟地址，实际上两个进程中的Num的绝对地址是不同的，因而父子进程写入不会相互影响。

**调用system()与exec族函数**：
代码：


```c
//showpid.c
#include <stdio.h>
#include <unistd.h>
int main() {
    // 获取当前进程的PID
    pid_t pid = getpid();
    // 获取当前进程的父进程的PID
    pid_t ppid = getppid();
    printf("Current PID: %d\n", pid);
    printf("Parent PID: %d\n", ppid);
    return 0;
}

```



```c
//system调用
#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
int main()
{
pid_t pid,pid1;
        pid = fork();

        if(pid <0){
            fprintf(stderr, "Fork Failed\n");
            return 1;
        }
        else if(pid == 0){
            int ret = sys("./showpid");
            printf("system return value: %d\n",ret);
        }
        else{
            int status;
            wait(&status);
            printf("child exit status: %d\n",status);
            exit(0);

        }

        return 0;
}

```


运行结果:(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/Psys.png)

System函数不会替换当前进程，只是调用系统的命令并返回命令的状态，子进程和父进程分别调用systeme函数进而调用程序，再将被调用程序执行结果返回，继续执行当前进程,因此sys函数调用后会输出ret状态。


代码：


```c
//exec族函数调用
#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
int main()
{
pid_t pid,pid1;
        pid = fork();
        if(pid <0){
            fprintf(stderr, "Fork Failed\n");
            return 1;
        }
        else if(pid == 0){

            int ret = excel("./showpid", "showpid", NULL);
            printf("exec return value:%d\n",ret);
            perror("excel");
            exit(1);
        }
        else{
            int status;
            wait(&status);
            printf("child exit status: %d\n",status);
            exit(0);

        }

        return 0;
}

```

运行结果：(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/Pexec.png)

exec会将当前进程替换为新的程序继续执行，子进程和父进程分别执行exec函数，两个进程都替换为新的程序执行，由于实现了进程替换，子进程抛弃掉原进程，因而再excel函数以后的部分不在执行，因此不输出ret状态。


## 线程相关编程实验

**两子线程对于共享变量分别循环5000次做不同操作并输出结果**

代码：

```c
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

```

运行结果：(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/T1.png)

两线程不存在任何同步机制，在运行的时间顺序上是随机的，两线程产生竞争现象，因而多次运行结果不会完全相同。当然也不排除一些特殊情况即其中一个线程运行完毕后，另一个线程继续运行，此时我们最终可以得到一个正确结果。


**使用PV操作实现同步和互斥**

代码：
```c
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

```

运行结果：(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/Tsignal.png)

如运行结果所示，在加入signal并进行PV操作后，确保了同一时间只有一个线程工作，不会出现竞争现象，因此最后得到正确结果0。

**调用System和exec族函数输出tid与pid**

代码：

```c
//tread.c
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>

int main(){
        pid_t pid = getpid();

        printf("Current pid = %d \n",pid);

        return 0;
}
```

```c
//system调用
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

```

运行结果：(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/Tsys.png)

在原进程创建的两个线程共享一个进程，因而两线程得到的进程号相同；同时线程id与进程id统一编号，不同线程又具有不同tid，因而两线程的tid在原进程的基础上分别加1和加2；而两线程在调用system函数时，分别创建了一个新的进程，因而创建了两个不同的进程，pid也因此不同。


代码：

```c
//exec族函数调用
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/syscall.h>
void thread_func(){
    int ret =execl("./thread", "thread", NULL);
    perror("excel");
   
}
void *thread1(){
    pid_t pid = getpid();
    pid_t tid = syscall(SYS_gettid);

    printf("Thread1: tid = %d pid = %d \n",tid,pid);
    thread_func();
    pthread_exit(NULL);
}

void *thread2(){
    pid_t pid=getpid();
    pid_t tid=syscall(SYS_gettid);

    printf("Thread2: tid = %d pid = %d \n",tid,pid);
    thread_func();
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

```

运行结果：(https://gitee.com/riac00/oslab1_img/blob/Lab_1img/img/Texec.png)

根据输出结果，只有一个线程运行并创建了新进程，这是因为excel函数会将当前进程替换为另一个可执行程序，而两线程共处一个进程之下，因而当其中一个线程调用了excel函数后，都会先将当前进程替换为另一个可执行程序，并在新程序中继续运行。这意味着原来的进程就不存在了。


**问题二**: 在进行线程相关实验时，利用gcc编译时链接失败，经查询发现需要加入 -lpthread 标志来链接 pthread库，问题解决。

**问题三**：在编写获取线程tid程序时，无法使用gettid函数，故将其替换为syscall(SYS_gettid)以实现相同的效果。

## 自旋锁相关实验


代码：

```c
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

```

运行结果：(https://gitee.com/riac00/oslab1_img/raw/Lab_1img/img/Tmutex.png)

由运行结果得，两线程操作中加入互斥锁后，最终输出结果始终正确，确保了同一时间只有一个线程工作，消除了竞争现象。

# 题目完成过程中遇到的问题及解决方法

## 问题1

**问题描述**

在进程相关实验时，在printf函数中是否加入\n会导致输出结果不同

**解决方法**

在对wait函数作用分析的实验中删除printf函数中的\n

## 问题2

**问题描述**

线程相关实验时正常gcc编译链接失败

**解决方法**

编译时在末尾加入-lpthread以链接到-pthread库

## 问题3

**问题描述**

在线程相关实验时无法调用gettid()函数输出线程的tid值

**解决方法**

将gettid()函数替换为syscall(SYS_gettid)以获取当前线程的tid值

# 题目完成过程中最得意的和收获最大内容

## 最得意处

在线程相关实验中分别在两线程结束处打印 terminated，并输出当前线程结束时共享变量的值，这让我更好的理解了线程的运行工作机制。

## 收获最大内容

1. exec族函数会实现进程替换，在进程相关实验时，由于父子进程是两个进程，因此一个进程调用exec函数不会影响另一进程；而线程则是在进程以下，多个线程共享一个进程，因此一旦某一线程调用exec族函数，整个进程就会替换到另一程序，所有线程都会被终止。

2. 本次实验让我较为深刻的体会到了线程之间的工作机制