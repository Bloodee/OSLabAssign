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