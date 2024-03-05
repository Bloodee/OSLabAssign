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