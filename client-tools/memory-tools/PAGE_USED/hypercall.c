#include <stdio.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <sys/ioctl.h>  
#include <sys/types.h>  
#include <fcntl.h>  
#include <string.h> 
#include <xenctrl.h>
#include <xen/sys/privcmd.h> 
#include <unistd.h>
#include <signal.h>

#define SHOW_PERIOD (1000*1000) //micro seconds
#define OUTPUT_PERIOD 1//seconds



int fd, ret, i, domID;
void end_process(){
    privcmd_hypercall_t hcall_2 = {  //end sample
        __HYPERVISOR_change_ept_content, 
        { domID, 0, 0, 2, 0}
    };  
    ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hcall_2); //stop sample
    if(ret==-1)
        printf("Hypercall error\n");
    else
        printf("end sampling\n");
    exit(0);
}

int main(int argc, char *argv[])  
{    
    double percent_threshold, percent;
    int monitor_time, sample_times;
    unsigned long *buff;
    buff = malloc(sizeof(unsigned long)*5);
    buff[0] = 0;
    buff[1] = 1;

    if(argc!=4){
        printf("Invalid arguments\n");
        exit(1);
    }

    signal(SIGINT, end_process); 

    domID = atoi(argv[1]);
    percent_threshold = atof(argv[2]);
    monitor_time = atoi(argv[3]);
    
    sample_times = (monitor_time*1000*1000)/SHOW_PERIOD;


    //Hypercall Structure    
    privcmd_hypercall_t hcall_1 = { //start sample 
        __HYPERVISOR_change_ept_content, 
        { domID, 0, 0, 1, 0}
    };  
    privcmd_hypercall_t hcall_3 = {  //get used
        __HYPERVISOR_change_ept_content, 
        { domID, 0, 0, 3, buff}
    };  

    fd = open("/proc/xen/privcmd", O_RDWR);  
    if (fd < 0) {  
        perror("open");  
        exit(1);  
    } 



    int output_period = OUTPUT_PERIOD*1000*1000/SHOW_PERIOD;
    ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hcall_1); // start sample
    if(ret==-1)
        printf("Hypercall error\n");
    else
        printf("Start monitoring please input CTRL+C to stop monitoring\n");


    while(sample_times > 0){
        percent = 0;
        ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hcall_3); //get used percent
        if(ret==-1)
            printf("Hypercall error\n");
        else{
            if(buff[1] != 0){
                percent = (double)buff[0]/(double)buff[1] * (double)100;
                if(output_period==0){
                    printf("Memory usage %f\%\n", percent);
                    output_period = OUTPUT_PERIOD*1000*1000/SHOW_PERIOD;
                }
            }
            else
                printf("there are some error total_pages is 0\n");
            output_period--;
        }
        if(percent > percent_threshold){
            printf("Maybe have Bottleneck!!!Memory usage is %f\% over your threshold\n", percent);
            break;
        }
        else{
            sample_times--;
            usleep(SHOW_PERIOD);
        }
    }

    end_process();


    return 0;

}  
