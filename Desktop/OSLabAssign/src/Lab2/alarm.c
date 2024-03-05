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
