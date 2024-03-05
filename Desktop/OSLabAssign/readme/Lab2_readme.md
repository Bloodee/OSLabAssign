# 题目完成情况

## 进程中的软中断通信

**man命令查看fork、kill、signal、sleep、exit手册**

**查看结果**

fork：(https://gitee.com/riac00/OS/raw/master/img/man_fork.jpg)
kill:(https://gitee.com/riac00/OS/raw/master/img/man_kill.jpg)
signal:(https://gitee.com/riac00/OS/raw/master/img/man_signal.jpg)
sleep:(https://gitee.com/riac00/OS/raw/master/img/man_sleep.jpg)
exit:(https://gitee.com/riac00/OS/raw/master/img/man_exit.jpg)

**根据流程图（如图 2.1 所示）编制实现软中断通信的程序**

运行结果：(https://gitee.com/riac00/OS/raw/master/img/2.1.2KILL1617.jpg)

我最初认为运行结果是 3/2 stop test在最前，17 stop test 在 Child process 2 is killed 前，16 stop test 在 Child process 1 is killed 前，parent process is killed 在最后，其余顺序可以随意出现。

实际结果大致呈现一个拓补结构，与我预想的大致相同，唯一有问题的一点是不会出现以下两种运行结果

(https://gitee.com/riac00/OS/raw/master/img/extoutput.jpg)

在运行结果看来，两种中断包括5s后中断所形成的效果是相同的，如果不给予软中断信号，父进程在5s后会向子进程发送kill信号，子进程打印输出并退出；当给予^C或^\，父进程想要进行中断，父程序向子进程发送kill信号，并等待子进程结束后终止该进程。

代码：

```c
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<signal.h>

int flag =0;
void inter_handler(int sig){
    switch(sig)
    {
        case SIGINT:printf("\n2 stop test\n");break;
        case SIGQUIT:printf("\n3 stop test\n");break;
        case SIGSTKFLT:printf("16 stop test\n");break;
        case SIGCHLD:printf("17 stop test\n");break;
    }
}
void waiting(int status){

}
int main(){
    int status;
    pid_t pid1=-1;pid2=-1;
    while(pid1 ==-1) pid1 = fork();
    if(pid>0){
        while(pid2==-1)pid2=fork();
        if(pid2>0){
            signal(SIGINT,inter_handler);
            signal(SIGQUIT,inter_handler);
            kill(pid1,16);
            kill(pid2,17);
            sleep(5);
            wait(0);
            wait(0);
            printf("\nParent process is killed!!\n");
        }
        else{
            signal(SIGINT,SIG_IGN);
            signal(SIGQUIT,SIG_IGN);
            signal(17,inter_handler);
            pause();
            printf("\nChild process2 is killed by parent!!\n");
            exit(0);
        }
    }
    else{
        signal(SIGINT,SIG_IGN);
        signal(SIGQUIT,SIG_IGN);
        signal(16,inter_handler);
        pause();
        printf("\nChild process1 is killed by parent!!\n");
        exit(0);
    }
    return 0;
}
```

**通过14号信号值进行闹钟终端**

闹钟中断运行结果：(https://gitee.com/riac00/OS/raw/master/img/alarm.jpg)

改为闹钟中断后，到达所设时钟时间值时，向父进程发送时钟中断信号，父进程处理后向子进程发送kill信号，并等待子进程终止。这种做法类似于前面等待休眠5s后自动运行kill命令，但通过设置时钟信号，可以对进程进行异步的终止，以更好的控制进程运行。

代码：
```c
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<signal.h>

int flag =0;
void inter_handler(int sig){
    switch(sig)
    {
        case SIGINT:printf("\n2 stop test\n");break;
        case SIGQUIT:printf("\n3 stop test\n");break;
        case SIGSTKFLT:printf("16 stop test\n");break;
        case SIGCHLD:printf("17 stop test\n");break;
    }
}
void waiting(int status){

}
int main(){
    int status;
    pid_t pid1=-1;pid2=-1;
    while(pid1 ==-1) pid1 = fork();
    if(pid>0){
        while(pid2==-1)pid2=fork();
        if(pid2>0){
            signal(SIGALRM,inter_handler);
            int left1 = 2;
            alarm(left1);
            printf("\n After %d, send alarm signal to process1\n",left1);
            sleep(2);
            kill(pid1,16);
            wait(0);
            int left2 = 4;
            alarm(left1);
            printf("\n After %d, send alarm signal to process2\n",left2);
            sleep(4);
            kill(pid2,17);
            wait(0);
            printf("\nParent process is killed!!\n");
        }
        else{
            signal(SIGINT,SIG_IGN);
            signal(SIGQUIT,SIG_IGN);
            signal(SIGALRM,SIG_IGN);
            signal(17,inter_handler);
            pause();
            printf("\nChild process2 is killed by parent!!\n");
            exit(0);
        }
    }
    else{
        signal(SIGINT,SIG_IGN);
        signal(SIGQUIT,SIG_IGN);
        signal(SIGALRM,SIG_IGN);
        signal(16,inter_handler);
        pause();
        printf("\nChild process1 is killed by parent!!\n");
        exit(0);
    }
    return 0;
}
```

kill命令在程序中使用了两次，作用是向子进程发送中断信号结束此进程，第一次执行后，子进程接受16信号并打印16 stop test，并打印子进程1被杀死的信息，第二次执行同理，打印17 stop test，并打印子进程2被杀死的信息。

进程可以通过exit或return函数来自主退出，进程自主退出的方式会更好一些，这样可以保证进程数据的完整性，使用kill命令在进程外部强制其退出可能导致进程的数据丢失或资源泄漏，但当进程出现异常无法退出时，可能需要kill命令来中止进程。

## 进程的管道通信

**man命令查看管道创建、同步互斥系统调用**

**查看结果**

pipe:(https://gitee.com/riac00/OS/raw/master/img/man_pipe.jpg)
mutual exclusion：(https://gitee.com/riac00/OS/raw/master/img/man_pthread.jpg)

**根据流程图和所给管道通信程序，补充代码，运行程序**

运行结果：
(1)有锁(https://gitee.com/riac00/OS/raw/master/img/pipemut.jpg)
(2)无锁(https://gitee.com/riac00/OS/raw/master/img/pipeNmut.jpg)

在有锁状态下，我认为会先打印2000个1，再打印2000个2，或者先打印2000个2，再打印2000个1；在无锁状态下，我认为1和2会交错打印。

实际结果：有锁状态下，同一时间内只有一个进程可以写入，因而是先写入了2000个1而后写入了2000个2，最后打印结果如图；无锁状态下，两进程对InPipe的写入在时序上是随机的，因而打印结果也是1与2交错打印。


同步与互斥的实现：在对管道写入前，先对管道用lockf函数加锁，等管道写入完毕后，在将管道解锁。

不控制的后果：由于多个进程同时向管道中写入数据，那么数据就很容易发生交错和覆盖，导致数据错误。

代码：

```c
#include <unistd.h> 
#include <signal.h> 
#include <stdio.h> 
#include<sys/wait.h>
#include<stdlib.h>
int pid1,pid2; // 定义两个进程变量
int main() {
    int fd[2]; 
    char InPipe[1000]; // 定义读缓冲区
    char c1='1', c2='2'; 
    pipe(fd); // 创建管道
    while((pid1 = fork( )) == -1); // 如果进程 1 创建不成功,则空循环
    if(pid1 == 0) { // 如果子进程 1 创建成功,pid1 为进程号
        lockf(fd[1],1,0);// 锁定管道
        
        for(int i=0;i<2000;i++)
            write(fd[1],&c1,1);// 分 2000 次每次向管道写入字符’1’ 
    sleep(5); // 等待读进程读出数据
    lockf(fd[1],0,0);// 解除管道的锁定
    exit(0); // 结束进程 1 
    } 
    else { 
    while((pid2 = fork()) == -1); // 若进程 2 创建不成功,则空循环
    if(pid2 == 0) { 
    lockf(fd[1],1,0); 
        for(int i=0;i<2000;i++)
            write(fd[1],&c2,1);// 分 2000 次每次向管道写入字符’2’ 
    sleep(5); 
    lockf(fd[1],0,0); 
    exit(0); 
    } 
    else { 
    wait(0);// 等待子进程 1 结束
    wait(0); // 等待子进程 2 结束
    read(fd[0],InPipe,4000);// 从管道中读出 4000 个字符
    InPipe[4000] = '\0';// 加字符串结束符
    printf("%s\n",InPipe); // 显示读出的数据
    exit(0); // 父进程结束
    }
}
```

## 内存的分配与回收

**算法比较**

算法思想：FF是从空闲分区表的第一个表目开始查找，将最先能够满足要求的空闲分区分配出去；BF是从全部空闲区中找出满足条件且大小最小的空闲分区并分配；WF是从空闲区中找出最大的满足条件的空闲分区并分配。

优缺点：FF算法能够把最先满足要求的空闲区分配出去，减少查找时间，并保留了高址部分的大空闲区，为将来可能到达的大作业的内存分配创造了条件，但是FF算法使得低址部分的空闲内存不断被划分，留下许多小的难以利用的空闲区，而每次查找都会从最低低址开始查起，增加了查找开销；
BF算法可以使碎片尽量小，尽可能保留大的空闲区，但是需要对空闲区进行按大小的排序，而且在不断利用的过程中，会产生大量不相邻的小空闲区，降低内存的利用率。
WF算法可以保留小的空闲区，减少小碎片产生，然而它需要对空闲区进行排序，且会破坏大空闲区，导致大作业难以分配内存。

提高查找性能：
    对于FF：选择合适的数据结构来对空闲块进行操作，同时采取内存紧缩技术来合并相邻的空闲分区。
    对于WF及BF：采取更高效的排序算法，以及通过FF算法来采取内存紧缩技术以提高内存利用率。
**空闲块排序**
    FF算法：按照空闲块首地址由低到高进行排序。
    BF算法：按照空闲块size大小由低到高进行排序。
    WF算法：按照空闲块size大小由高到低进行排序。
**内碎片、外碎片及紧缩功能**
    内碎片：本次实验的实现是通过动态分配和释放内存，唯一产生内碎片的点是当分配内存到所在空闲块后，空闲块剩余大小小于等于MIN_SLICE时，该内存会把这整片空闲块占据，而多出的那一小部分即是内碎片，如下图所示：(https://gitee.com/riac00/OS/raw/master/img/inslice.jpg)
    外碎片:多个进程被动态分配和释放时，会产生不连续的空闲分区，如下图所示：(https://gitee.com/riac00/OS/raw/master/img/exslice.jpg)
    内存紧缩：通过将内存连续的外碎片合并到一起，来实现内存紧缩，如下图所示：(https://gitee.com/riac00/OS/raw/master/img/MemoeryJinSuo.jpg),当进程7被删除时，其两侧的外碎片会被紧缩到一起。
**回收内存时空闲块的合并**
    当回收内存时，先将该分配块变成空闲块并把它分配到原空闲块的头节点上，再调用FF算法将空闲块按地址从小到大排序，最后在从头遍历，当前一个节点的start_addr+size = 后一个节点的start_addr时合并两个空闲块。
    
代码：

```c
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#define PROCESS_NAME_LEN 32
#define MIN_SLICE 10 
#define DEFAULT_MEM_SIZE 1024 
#define DEFAULT_MEM_START 0 

#define MA_FF 1
#define MA_BF 2
#define MA_WF 3
int mem_size=DEFAULT_MEM_SIZE; 
int ma_algorithm = MA_FF; 
static int pid = 0; 
int flag = 0;

struct free_block_type{
 int size;
 int start_addr;
 struct free_block_type *next;
}; 

struct free_block_type *free_block;

struct allocated_block{
 int pid; int size;
 int start_addr;
 char process_name[PROCESS_NAME_LEN];
 struct allocated_block *next;
 };

struct allocated_block *allocated_block_head = NULL;

struct free_block_type *init_free_block(int mem_size){
    struct free_block_type *fb;
    fb=(struct free_block_type *)malloc(sizeof(struct free_block_type));
    if(fb==NULL){
        printf("No mem\n");
        return NULL;
    }
    fb->size = mem_size;
    fb->start_addr = DEFAULT_MEM_START;
    fb->next = NULL;
    return fb;
}
void do_exit(){
	while(free_block->next!= NULL&&free_block !=NULL){
		struct free_block_type *temp = free_block;
		free_block = free_block->next;
		free(temp);
	}
	if(free_block)
		free(free_block);
}

void display_menu(){
    printf("\n");
    printf("1 - Set memory size (default=%d)\n", DEFAULT_MEM_SIZE);
    printf("2 - Select memory allocation algorithm\n");
    printf("3 - New process \n");
    printf("4 - Terminate a process \n");
    printf("5 - Display memory usage \n");
    printf("0 - Exit\n");
}
int set_mem_size(){
    int size;
    if(flag!=0){ 
    printf("Cannot set memory size again\n");
    return 0;
    }
    printf("Total memory size =");
    scanf("%d", &size);
    if(size>0) {
        mem_size = size;
        free_block->size = mem_size;
    }
    flag=1; return 1;
}

int rearrange_FF(){ 
    struct free_block_type *fbt = free_block;
    struct free_block_type *cfb;
     while(fbt->next != NULL){
        cfb = fbt->next;
        while(cfb != NULL){
            if(fbt->start_addr > cfb->start_addr){
            
                int temp_size = fbt->size;
                int temp_start = fbt->start_addr;
                fbt->size = cfb->size;
                fbt->start_addr = cfb->start_addr;
                cfb->size = temp_size;
                cfb->start_addr = temp_start;
            }
            cfb = cfb->next;
        }
        fbt = fbt->next;
     }
     return 1;
}
    
int rearrange_BF(){
     struct free_block_type *fbt = free_block;
     struct free_block_type *cfb;
     while(fbt->next != NULL){
         cfb = fbt->next;
         while(cfb != NULL){
             if(fbt->size > cfb->size){
               
                 int temp_size = fbt->size;
                 int temp_start = fbt->start_addr;
                 fbt->size = cfb->size;
                 fbt->start_addr = cfb->start_addr;
                 cfb->size = temp_size;
                 cfb->start_addr = temp_start;
             }
             cfb = cfb->next;
         }
         fbt = fbt->next;
     }
	return 1;
}

int rearrange_WF(){
     struct free_block_type *fbt ;
	 fbt = free_block;
     struct free_block_type *cfb;
     while(fbt->next != NULL){
         cfb = fbt->next;
         while(cfb != NULL){
             if(fbt->size < cfb->size){

                 int temp_size = fbt->size;
                 int temp_start = fbt->start_addr;
                 fbt->size = cfb->size;
                 fbt->start_addr = cfb->start_addr;
                 cfb->size = temp_size;
                 cfb->start_addr = temp_start;
             }
             cfb = cfb->next;
         }
         fbt = fbt->next;
     }
}

int rearrange(int algorithm){
    switch(algorithm){
    case MA_FF: return rearrange_FF(); break;
    case MA_BF: return rearrange_BF(); break;
    case MA_WF: return rearrange_WF(); break;
 }
}

void set_algorithm(){
    int algorithm;
    printf("\t1 - First Fit\n");
    printf("\t2 - Best Fit \n");
    printf("\t3 - Worst Fit \n");
    scanf("%d", &algorithm);
    if(algorithm>=1 && algorithm <=3) 
        ma_algorithm=algorithm;

    rearrange(ma_algorithm); 
}

int allocate_mem(struct allocated_block *ab){
 	struct free_block_type *fbt, *pre;
 	int request_size=ab->size;
 	fbt = pre = free_block;
	while(fbt != NULL){
		if(fbt->size >= request_size){ 
			if(fbt->size - request_size <= MIN_SLICE){ 
		  		ab->start_addr = fbt->start_addr;
		 		ab->size = fbt->size;
				if(fbt == free_block){ 
					free_block = fbt->next; 
				}
				else{
					 pre->next = fbt->next; 
				}
				
			    free(fbt); 

		}
			else{ 
			    ab->start_addr = fbt->start_addr;
			    ab->size = request_size;
			    fbt->start_addr += request_size; 
			    fbt->size -= request_size; 
		    }
		rearrange(ma_algorithm);
	    return 1; 
	   }
    pre = fbt; 
	fbt = fbt->next; 
 	}
	if(rearrange(ma_algorithm) == 1){ 
		pre = fbt = free_block; 
		while(fbt != NULL){
			if(fbt->size >= request_size){ 
			if(fbt->size - request_size <= MIN_SLICE){ 
		  		ab->start_addr = fbt->start_addr;
		 		ab->size = fbt->size;
				if(fbt == free_block){ 
				    
					    free_block = fbt->next; 
					}
					else{
					    pre->next = fbt->next; 
					}
			    free(fbt);
			    }
			 else{ 
				    ab->start_addr = fbt->start_addr;
				    ab->size = request_size;
				    fbt->start_addr += request_size; 
				    fbt->size -= request_size; 
			    }
			    rearrange(ma_algorithm);
			    return 1;
		    }
             else if(fbt->size < MIN_SLICE){ 
                return -1; 
	}
		    pre = fbt; 
		    fbt = fbt->next; 
	  	}	
 	}
 return -1; 
}


int new_process(){
    struct allocated_block *ab;
    int size; int ret;
    ab=(struct allocated_block *)malloc(sizeof(struct allocated_block));
    if(!ab) exit(-5);
    ab->next = NULL;
    pid++;
    sprintf(ab->process_name, "PROCESS-%02d", pid);
    ab->pid = pid; 
    printf("Memory for %s:", ab->process_name);
    scanf("%d", &size);
    if(size>0) ab->size=size;
    ret = allocate_mem(ab);
    if((ret==1) &&(allocated_block_head == NULL)){ 
        allocated_block_head=ab;
        return 1; }

    else if (ret==1) {
        ab->next=allocated_block_head;
        allocated_block_head=ab;
        return 2; }
    else if(ret==-1){ 
    	pid--;
        printf("Allocation fail\n");
        free(ab);
        return -1; 
    }
    return 3;
}


struct allocated_block *find_process(int pid){
    struct allocated_block *ab = allocated_block_head; 
    while(ab != NULL){ 
        if(ab->pid == pid){ 
            return ab; 
        }
        ab = ab->next; 
    }
    return NULL; 
}



int free_mem(struct allocated_block *ab){
    int algorithm = ma_algorithm;
    struct free_block_type *fbt, *pre;
    fbt=(struct free_block_type*) malloc(sizeof(struct free_block_type));
    if(!fbt) return -1;

    fbt->size=ab->size;
    fbt->start_addr=ab->start_addr;
    fbt->next=free_block;
    free_block = fbt;
    
	rearrange_FF();
	pre = free_block;
	struct free_block_type *next = pre->next;
    
    while(next != NULL){  // Modify this line
        if(pre->start_addr + pre->size == next->start_addr){ 
            pre->size += next->size; 
            struct free_block_type *temp = next;
            pre->next = next->next; 
            free(temp); 
            next = pre->next;  // Add this line
        }
        else{ 
            pre = pre->next; 
            next = next->next;  // Add this line
        }
    }
    rearrange(ma_algorithm);
    return 0;
}

int dispose(struct allocated_block *free_ab){
    struct allocated_block *pre, *ab;
    if(free_ab == allocated_block_head) { 
    allocated_block_head = allocated_block_head->next;
    free(free_ab);
    return 1;
    }
    pre = allocated_block_head; 
    ab = allocated_block_head->next;
    while(ab!=free_ab){ pre = ab; ab = ab->next; }
    pre->next = ab->next;
    free(ab);
    return 2;
 }

void kill_process(){
    struct allocated_block *ab;
    int pid;
    printf("Kill Process, pid=");
    scanf("%d", &pid);
	ab = find_process(pid);
	if(ab!=NULL){
        free_mem(ab); 
        dispose(ab);
        printf("\nProcess %d has been killed\n", pid); 
    }
    else
        printf("\nThere is no Process %d\n",pid);
}

int display_mem_usage(){
    struct free_block_type *fbt= free_block;
    struct allocated_block *ab=allocated_block_head;
    printf("----------------------------------------------------------\n");
    printf("Free Memory:\n");
    printf("%20s %20s\n", " start_addr", " size");
    while(fbt!=NULL){
        printf("%20d %20d\n", fbt->start_addr, fbt->size);
        fbt=fbt->next;
 } 

 printf("\nUsed Memory:\n");
 printf("%10s %20s %10s %10s\n", "PID", "ProcessName", "start_addr", " size");
 while(ab!=NULL){
    printf("%10d %20s %10d %10d\n", ab->pid, ab->process_name, 
    ab->start_addr, ab->size);
    ab=ab->next;
 }
 printf("----------------------------------------------------------\n");
 return 0;
 }

int main(){
    char choice; pid=0;
    free_block = init_free_block(mem_size); 
    while(1) {
    display_menu(); 
        fflush(stdin);
        choice=getchar(); 
        switch(choice){
            case '1': set_mem_size(); break; 
            case '2': set_algorithm();flag=1; break;
            case '3': new_process(); flag=1;break;
            case '4': kill_process(); flag=1; break;
            case '5': display_mem_usage(); flag=1; break; 
            case '0': do_exit(); exit(0); 
            default: break; 
            } 
        } 
    return 0;
}
```

## 页面的置换

部分运行结果：(https://gitee.com/riac00/OS/raw/master/img/LRU&FIFO.jpg)

**实现与性能**
实现：FIFO，当需要进行页面替换时，最先进入的页面被替换；LRU，当需要进行页面替换时，最久未被访问的页面最先被替换，这就导致FIFO算法相比LRU算法更简单，更容易实现。

性能：
测试对比图 
页数量固定，改变内存块数量(https://gitee.com/riac00/OS/raw/master/img/FL_blocknum.jpg)
内存块数量固定，改变页数量：(https://gitee.com/riac00/OS/raw/master/img/FL_pagenum.jpg)
如上两张图片所示，当物理内存大小固定时，页面序列数量的增加对于两种算法missing rate的影响大致相同；当页面数量固定时，物理内存增加两算法missing rate都降低，但对于FIFO算法，物理内存block为20时的missing rate却要比15时要更高，这体现出了FIFO的Belady现象，然而LRU算法仍旧有着很好的性能。因而，LRU算法在性能方面优于FIFO算法，它能更好的利用页面的局部性原理，减少缺页率，提高命中率；而FIFO算法却容易淘汰掉一些频繁访问的页面，使得缺页率增加，命中率低。

**局部性实现**
每当引用新的页面时，都会将该页面移至内存分配序列的头部，并将其他页面依次向后移一位，每当需要访问页面时会首先访问头节点，而头结点的页面一定是最近刚刚访问过的页面，而最不常访问的会被放在尾端，保证了算法实现的局部性。

**提高内存利用率**
1、使用合适的页面置换算法
2、使用动态分区分配，搭配内存紧缩技术
3、使用虚拟内存技术

FIFO算法
```c
#include<stdio.h>
#include<stdlib.h>
#define PAGEMAX 100
typedef struct page{
	int num;
	int status;
	struct page *next;
}page;

page *head;
page *tail;

int repos=0;

int pagenum;
int blocknum;
int *page_seq;
int missingcount;

void init(){
	int i;
	head = (page *)malloc(sizeof(page));
	tail=head;
    head->num = 0;
    head->status = 0;
    page *Pp[blocknum];
    for(i =0; i < blocknum-1;i++){
    	Pp[i] = (page *)malloc(sizeof(page));
        Pp[i]->num = 0;
        Pp[i]->status = 0;
	}
	for(i =0;i<blocknum-2;i++){
		Pp[i]->next = Pp[i+1];
	}
	if(Pp[0])
		head->next =Pp[0];
	if(blocknum>=2)
		tail = Pp[blocknum-2];
	tail->next=NULL;
}

void print(){
	page *p = head;
    printf("Page ：");
    while (p != NULL) {
        printf("%d ", p->num);
        p = p->next;
    }
    printf("\n");
    printf("The poisition of head：%d\n", head->num);
    printf("The poisition of tail：%d\n", tail->num);
    printf("\n");
}

int search(int p) {
    page *q = head;
    while (q != NULL) {
        if (q->num == p) {
            return 1;
        }
        q = q->next;
    }
    return 0;
}

page *find_free() {
    page *q = head;
    while (q != NULL) {
        if (q->status == 0) {
            return q;
        }
        q = q->next;
    }
    return NULL;
}

void replace(int p) {
	int i;
	page *P =(page *)malloc(sizeof(page));
	P =head;
    for(i=0;i<repos;i++){
		P=P->next;
	}
	P->num = p;
	repos = (repos+1)%blocknum;
}

void simulate(){
	int i;
	for(i=0;i<pagenum;i++){
		printf("serch page: %d\n",page_seq[i]);
		if(search(page_seq[i])){
			printf("page hit! \n");
		}
		else{
			printf("page missing \n");
			missingcount++;
			page *fb = find_free();
			if(fb){
				printf("There is free block, fill it in\n");
				fb->num = page_seq[i];
                fb->status = 1;
			}
			else{
				printf("No free block, replace\n");
				replace(page_seq[i]);
			}
			print();
		}
	}
}

int main(){
	int i;int random=0;
	printf("input page number：\n");
    scanf("%d", &pagenum);
    printf("input block number：\n");
    scanf("%d", &blocknum);
    page_seq = (int *)malloc(sizeof(int) * pagenum);
    printf("choose the way of creating page sequence\n");
	printf("input 1 for random sequence, 0 for your own sequence\n");
	
	scanf("%d",&random);
	if(random){
		printf("\nrandom page sequence:");
		for ( i = 0; i < pagenum; i++) {
	        page_seq[i] = rand() % ((blocknum) + 2)+1;
	        printf("%d ", page_seq[i]);
	    }
	    printf("\n");
	}
	else{
    	printf("input page sequence: ");
	    for ( i = 0; i < pagenum; i++) {
	        scanf("%d", &page_seq[i]);
	    }
	}
	
    init();
    simulate();
    printf("page missing number：%d\n", missingcount);
    printf("page missing rate：%.2f%%\n", (double)missingcount / pagenum * 100);
    return 0;
}
```

LRU算法
```c
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node* next;
} Node;

int cpn=0;
int pageMissCount = 0;

Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

void insertAtHead(Node** head, int data) {
    Node* newNode = createNode(data);
    newNode->next = *head;
    *head = newNode;
}

Node* findNode(Node* head, int data) {
    Node* current = head;
    while (current != NULL) {
        if (current->data == data) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void deleteNode(Node** head, int data) {
    Node* current = *head;
    Node* prev = NULL;

    while (current != NULL && current->data != data) {
        prev = current;
        current = current->next;
    }

    if (current != NULL) {
        if (prev == NULL) {
            *head = current->next;
        } else {
            prev->next = current->next;
        }
        free(current);
    }
}

void deleteTail(Node** head) {
    Node* current = *head;
    Node* prev = NULL;

    while (current != NULL && current->next != NULL) {
        prev = current;
        current = current->next;
    }

    if (current != NULL) {
        if (prev == NULL) {
            *head = NULL;
        } else {
            prev->next = NULL;
        }
        free(current);
    }
}

void printList(Node* head) {
    Node* current = head;
    while (current != NULL) {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

int main() {
    int i;
    int generate=0;
    int memorySize, pageSize;
    printf("input block number: ");
    scanf("%d", &memorySize);

    printf("input page number: ");
    scanf("%d", &pageSize);

    int* pages = (int*)malloc(sizeof(int) * pageSize);
	printf("choose the way of creating page sequence\n");
	printf("input 1 for random sequence, 0 for your own sequence\n");
	
	scanf("%d",&generate);
	
	if(generate){
		printf("\nrandom page sequence:");
		for ( i = 0; i < pageSize; i++) {
	        pages[i] = rand() % ((memorySize) + 2)+1;
	        printf("%d ", pages[i]);
	    }
	    printf("\n");
	}
	else{
    	printf("input page sequence: ");
	    for ( i = 0; i < pageSize; i++) {
	        scanf("%d", &pages[i]);
	    }
	}
    printf("\n");

    Node* memory = NULL;
    Node* pageList = NULL;

    for (i = 0; i < pageSize; i++) {
        int currentPage = pages[i];
		printf("serch for page %d\n", currentPage);
        if (findNode(memory, currentPage) != NULL) {
        	printf("page hit\n");
            deleteNode(&memory, currentPage);
            insertAtHead(&memory, currentPage);
        } else {
        	
		printf("page missing\n");	
        if (cpn>=memorySize) {
    		printf("page evict\n");       	
            deleteTail(&memory);
            cpn--;
        }
			
            insertAtHead(&memory, currentPage);
            cpn++;
            pageMissCount++;
        }

        printf("current page situation：");
        printList(memory);
        printf("\n");
    }

    printf("missing number: %d\n", pageMissCount);
    printf("missing rate: %.2f%%\n", (float)pageMissCount / pageSize * 100);

    free(pages);
    Node* current = memory;
    while (current != NULL) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    current = pageList;
    while (current != NULL) {
        Node* next = current->next;
        free(current);
        current = next;
    }

    return 0;
}
```