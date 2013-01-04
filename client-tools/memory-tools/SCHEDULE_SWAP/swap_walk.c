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

int fd, ret, ary_size, os_type, flag, domID;  



int main(int argc, char *argv[])  
{ 

    unsigned long total_diff, cr3;
    unsigned long total_swap, token;
    unsigned long *swap_num, *cr3_list, *touched_cr3, *tmp;
    int first; 

    long i, num;
    long j;

    if(argc!=5){
        printf("error arguments domID os cr3 first\n");
        exit(1);
    }
    domID = atoi(argv[1]);
    os_type = atoi(argv[2]);
	cr3 = strtoul(argv[3], NULL, 16); 
	first = atoi(argv[4]);


    fd = open("/proc/xen/privcmd", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }


	if(first == 1){
			privcmd_hypercall_t hyper0 = { //init environment 
				__HYPERVISOR_change_ept_content, 
				{ domID, os_type, 0, 13, 0}
			};
			ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper0); 
			privcmd_hypercall_t hyper1 = { //init hash table 
				__HYPERVISOR_change_ept_content, 
				{ domID, 0, 0, 9, 0}
			};
			ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper0); 
			if(ret == -1){
				printf("hash table alloc error\n");
				return -1;
			}
			first = 0;	
	}
    
	if(first == 0){
			privcmd_hypercall_t hyper0 = { //test 
				__HYPERVISOR_change_ept_content, 
				{ domID, cr3, 0, 11, 0}
			};
			ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper0); 
	}
    return 0;

}
