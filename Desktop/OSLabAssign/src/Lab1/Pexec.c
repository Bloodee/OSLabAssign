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