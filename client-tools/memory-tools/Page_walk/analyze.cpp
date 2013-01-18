#include <iostream>
extern "C"{
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
#include <string.h>
#include <sys/mman.h>
#include <xs.h>
#include <xen/hvm/save.h>
}
#include "analyze.h"
#include <set>
using namespace std;



int main(int argc, char *argv[])  
{ 
	int fd, ret, i, list_size;  
	unsigned long cr3, value, os_type, total_change_page, each_change_page;
	unsigned long offset = 0;
	struct hash_table global_hash;
	DATAMAP data_map;
	unsigned int round = 0;
	unsigned long cr3_list[30];
	unsigned long result[10];


	xch =  xc_interface_open(0,0,0);
	if(xch == NULL)
		printf("xch init error\n");

	if(argc<3){
		printf("%s domID os_type(linux->0 windows->1)\n", argv[0]);
		exit(1);
	}
	domID = atoi(argv[1]);
	os_type = atoi(argv[2]);

//	cr3 = (addr_t)strtoul(argv[1], NULL, 16);
//	fscanf(stdin, "%lx %lx %lx %lx %lx %lx", &cr3_list[0], &cr3_list[1], &cr3_list[2], &cr3_list[3], &cr3_list[4], &cr3_list[5]);
//	fscanf(stdin, "%lx %lx", &cr3_list[0], &cr3_list[1]);
//	list_size = 1;


	
	//Get cr3 from Hypervisor
	fd = open("/proc/xen/privcmd", O_RDWR);  
	if (fd < 0) {  
		perror("open");  
	  	exit(1);  
	}
    privcmd_hypercall_t hyper0 = { 
		  __HYPERVISOR_change_ept_content, 
		  { domID, 0, 0, 15, 0}
	};
    privcmd_hypercall_t hyper1 = { 
		  __HYPERVISOR_change_ept_content, 
		  { domID, 0, 0, 16, cr3_list}
	};
	

	while(1){
		for(i=0; i<list_size; i++)
			cr3_list[i] = 0;
		
		ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper0); 
		ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1);
		list_size = cr3_list[0];
		for(i=1; i<=list_size; i++){
			cr3_list[i-1] = cr3_list[i];
		}
		cr3_list[i+1] = 0;
		list_size = 5; //limit to size 5


		system_map_wks.clear();
		system_map_swap.clear();
		total_change_page = 0;
		each_change_page  = 0;

		walk_cr3_list(data_map, cr3_list, list_size, os_type, round);
		each_change_page = check_cr3_list(data_map, cr3_list, list_size);
		calculate_all_page(data_map, os_type, result);

		printf("Invalid Memory:%lu[M] Valid Memory:%lu[M] Total valid Memory:%lu[M] map size:%lu[M] round %d\n\n", 
					result[0]/256, result[1]/256, result[2]/256, data_map[cr3_list[0]].h.size()/(1024*1024), round);
		round++;
		retrieve_list(data_map, round);
		sleep(2);		
	}

	return 0;
}


