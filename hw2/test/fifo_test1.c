#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
void pp(){
    union wait status;
    printf("k\n");
    wait3(&status, WNOHANG, (struct rusage*)0);
    printf("kk\n");
}

void main(){

    signal(SIGCHLD,pp);
/*    int o_pid = getpid();
    int pid = fork();
    if(pid == 0){
//        sleep(5);
        kill(o_pid,SIGCHLD);      
        sleep(5); 
    }
    else{
        pause();
//        signal(SIGCHLD,pp);        
        sleep(10);
//        sleep(10);
        printf("ok %d now: %d\n",o_pid,getpid());
//        printf("why no stop\n");
    }*/

    mkfifo("FIFO",0666);
//    int fd = open ("FIFO", O_WRONLY);
    int fd = open ("FIFO", O_WRONLY|O_NONBLOCK);
//    int fd = open ("FIFO", O_RDWR|O_NONBLOCK, 0);
//    sleep(5);    
    char s[10] = "hello\0";
    int k;
    k = write(fd, s, strlen(s));    
    printf("write %d bytes %s to FIFO\n",k,s);
//sleep(5);
//    char recv[10];
//    read(fd, recv, 10);
//    printf("%s\n",recv);
//    sleep(5);
//    close(fd);
    
}
