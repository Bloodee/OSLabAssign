#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>

int main(){
        pid_t pid = getpid();

        printf("Current pid = %d \n",pid);

        return 0;
}