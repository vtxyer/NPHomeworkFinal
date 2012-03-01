#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

void main(){
/*    FILE *tmp_fp;
    tmp_fp = fopen("/tmp/FIFO/1","r");
    if(tmp_fp){
        fclose(tmp_fp);
        printf("exist\n");
    }
    else{
        printf("not exist\n");
    }
*/

    mkfifo("FIFO",0666);
//    int fd = open ("FIFO", O_RDONLY);
    int fd = open ("FIFO", O_RDONLY);    
//    int fd = open ("FIFO", O_RDONLY|O_NONBLOCK);
//    printf("%d\n",fd);   
    printf("read %d\n",fd);
    char a[20];
    sleep(5);
    read(fd, a, sizeof(a));
    printf("%s\n",a);
    close(fd);

}
