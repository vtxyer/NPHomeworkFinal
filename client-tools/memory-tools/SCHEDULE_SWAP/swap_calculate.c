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
#include <libvmi/libvmi.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

#define RECENT_CR3_SIZE 100+1

#define REFRESH_PERIOD 10
#define SHOW_PERIOD (200*1000) //micro seconds
#define SAMPLE_TIMES 30000
#define THRESHOLD 300
#define RESAMPLE_PERIOD 3
#define COPY_TO_SAMPLE 3


int fd, ret, ary_size, os_type, flag, domID;  

void end_process(){
    privcmd_hypercall_t hyper2 = { //empty environment 
        __HYPERVISOR_change_ept_content, 
        { domID, os_type, 0, 17, 0}
    };
    ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper2);
    privcmd_hypercall_t hyper3 = { //empty environment 
        __HYPERVISOR_change_ept_content, 
        { domID, os_type, 0, 14, 0}
    };
    ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper3);
    printf("over\n");
    exit(0);
}




int main(int argc, char *argv[])  
{ 

    unsigned long total_diff, max_swap;
    unsigned long tot_pages, tot_swap;
    unsigned long *swap_diff, *last_swap_diff, *cr3_list;
    long i;
    long j;

    signal(SIGINT, end_process);

    cr3_list = (unsigned long *)malloc(sizeof(unsigned long)*RECENT_CR3_SIZE);
    swap_diff = (unsigned long *)malloc(sizeof(unsigned long)*RECENT_CR3_SIZE);
    last_swap_diff = (unsigned long *)malloc(sizeof(unsigned long)*RECENT_CR3_SIZE);

    memset( cr3_list, 0, RECENT_CR3_SIZE);
    memset( swap_diff, 0, RECENT_CR3_SIZE);
    memset( last_swap_diff, 0, RECENT_CR3_SIZE);

    if(swap_diff ==NULL || last_swap_diff==NULL){
        printf("allocate error\n");
        exit(1);
    }

    if(argc!=3){
        printf("error arguments\n");
        exit(1);
    }
    domID = atoi(argv[1]);
    os_type = atoi(argv[2]);


    fd = open("/proc/xen/privcmd", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }


    privcmd_hypercall_t hyper0 = { //init environment 
        __HYPERVISOR_change_ept_content, 
        { domID, os_type, 0, 13, 0}
    };
    ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper0);    //init environment

    flag = 0;
    tot_swap = 0;
    max_swap = 0;
    int show_times, sample_times, bottleneck_flag;


    printf("Start monitoring please input CTRL+C to stop monitoring\n");


    if(os_type == 1){//WINDOWS
        while(1){
            privcmd_hypercall_t hyper6 = { //empty environment 
                __HYPERVISOR_change_ept_content, 
                { domID, os_type, 0, 17, 0}
            };
            ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper6);
            privcmd_hypercall_t hyper0 = { //refresh
                __HYPERVISOR_change_ept_content, 
                { domID, os_type, 0, 15, cr3_list}
            };
            ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper0);

            sleep(COPY_TO_SAMPLE);

            privcmd_hypercall_t hyper1 = { //re-sample
                __HYPERVISOR_change_ept_content, 
                { domID, os_type, 0, 18, swap_diff}
            };
            ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);

            sleep(REFRESH_PERIOD);           

            memset( swap_diff, 0, RECENT_CR3_SIZE);
            privcmd_hypercall_t hyper3 = { //copy swap_num to buff
                   __HYPERVISOR_change_ept_content, 
                   { domID, os_type, 0, 19, swap_diff}
            };
            ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper3);
            ary_size = swap_diff[0];
            tot_swap = 0;
            for(i=1; i<ary_size+1; i++){
                 tot_swap += swap_diff[i];
            }  
            if(tot_swap > max_swap && tot_swap>2560){
                max_swap = tot_swap;
                printf("Maybe you have to add extra:%lu[M] memory size\n", max_swap/256);
            }
        }
        return 0;
    }
    else if(os_type == 0 ){  //LINUX
        bottleneck_flag = 0;
        while( 1 ){    

            privcmd_hypercall_t hyper6 = { //empty environment 
                __HYPERVISOR_change_ept_content, 
                { domID, os_type, 0, 17, 0}
            };
            ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper6);
            for(i=0; i<RECENT_CR3_SIZE; i++){
                swap_diff[i] = 0;                
            }
            show_times = (REFRESH_PERIOD*1000*1000)/SHOW_PERIOD;

            privcmd_hypercall_t hyper0 = { //refresh
                __HYPERVISOR_change_ept_content, 
                { domID, os_type, 0, 15, cr3_list}
            };
            ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper0);

            sample_times = SAMPLE_TIMES;

            sleep(COPY_TO_SAMPLE);

RESAMPLE:{  

             ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper6);
             privcmd_hypercall_t hyper1 = { //re-sample
                 __HYPERVISOR_change_ept_content, 
                 { domID, os_type, 0, 18, swap_diff}
             };
             ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);

             while(show_times > 0){
                 for(i=0; i<RECENT_CR3_SIZE; i++){
                     //                last_swap_diff[i] = swap_diff[i];
                     swap_diff[i] = 0;                
                 }
                 privcmd_hypercall_t hyper2 = { //copy answer to buff
                     __HYPERVISOR_change_ept_content, 
                     { domID, os_type, 0, 16, swap_diff}
                 };
                 ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper2);

                 total_diff = 0;
                 ary_size = swap_diff[0];
                 for(i=1; i<ary_size+1; i++){
                     //                total_diff += swap_diff[i] - last_swap_diff[i];
                     total_diff += swap_diff[i];
                 }


                 if( total_diff > THRESHOLD )
                 {
                     printf("There are bottleneck %lu ", total_diff);
                     for(i=0; i<RECENT_CR3_SIZE; i++){
                         swap_diff[i] = 0;                
                     }
                     privcmd_hypercall_t hyper3 = { //copy swap_num to buff
                         __HYPERVISOR_change_ept_content, 
                         { domID, os_type, 0, 19, swap_diff}
                     };
                     ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper3);
                     tot_swap = 0;
                     ary_size = swap_diff[0];
                     for(i=1; i<ary_size+1; i++){
                         tot_swap += swap_diff[i];
                     } 
                     printf("you have to add extra %lu[M] memory\n", tot_swap/256);
                     sample_times--;
                     if(sample_times == 0){
                         printf("End caculate swap\n");
                         goto END;
                     }
                     sleep(RESAMPLE_PERIOD);
                     goto RESAMPLE;
                 }

                 show_times--;
                 usleep(SHOW_PERIOD);
             }
         }

        }
    }
END:{
        end_process();

        close(fd);
        free(swap_diff);
        free(last_swap_diff);
    }
    return 0;

}
