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
#define INVALID_GFN (-1UL)
#define RECENT_CR3_SIZE 100+1

#define REFRESH_PERIOD 10
#define SHOW_PERIOD (200*1000) //micro seconds
#define SAMPLE_TIMES 30000
#define THRESHOLD 300
#define SWAP_STORE_NUM 5


int fd, ret, ary_size, os_type, flag, domID;  


struct InfoNode{
    unsigned long cr3;
    unsigned long swap_num[SWAP_STORE_NUM];
    unsigned long valid[SWAP_STORE_NUM];
};
struct Cr3_queue{
   struct InfoNode *next;
   struct InfoNode info;
};
int deleteInfo(struct Cr3_queue *Cr3_queue, unsigned long cr3){
    struct Cr3_queue *tmp, *pre;
    if(Cr3_queue == NULL)
        return 0;
    pre = Cr3_queue;
    if(pre->info.cr3 == cr3){
        Cr3_queue = NULL;
        free(pre);
        return 1;
    }    
    tmp = Cr3_queue->next;
    while(tmp){        
        if(tmp->info.cr3 == cr3){
            pre->next = tmp->next;
            free(tmp);
            break;
        }
        pre = tmp;
        tmp = tmp->next;
    }
    return 1; 
}
int addInfo(struct Cr3_queue *queue, unsigned long cr3, unsigned long swap_num, int token){
    struct Cr3_queue *tmp;
    int i;
    tmp = queue;
    if(tmp == NULL){
        return 0;
    }
    else{
        while(tmp->next != NULL){
            tmp = tmp->next;
        }
        struct Cr3_queue *tmpInfo = (struct Info *)malloc(sizeof(struct Cr3_queue));
        for(i=0; i<SWAP_STORE_NUM; i++){
             queue->info.swap_num[i] = 0;
             queue->info.valid[i] = 0;
        }
        tmpInfo->info.cr3 = cr3;
        tmpInfo->info.swap_num[token] = swap_num;
        tmp->next = tmpInfo;
        tmpInfo->next = NULL;
        return 1;
    }
    return 0;
}
int updateInfo(struct Cr3_queue *queue, unsigned long cr3, unsigned long swap_num, int token){    
    if(queue == NULL){
        return 1;
    }
    else{
        struct Cr3_queue *tmp = queue;
        while(tmp){
            if(tmp->info.cr3 == cr3){
                tmp->info.swap_num[token] = swap_num;
                tmp->info.valid[token] = 0;
                break;
            }
            tmp = tmp->next;
        }
        addInfo(queue, cr3, swap_num, token); 
        return 1;
    }
    return 0;
}
int setValid(struct Cr3_queue *queue, unsigned long cr3, int token){
    if(queue == NULL){
        return 0;
    }
    else{
        struct Cr3_queue *tmp = queue;
        if(token < 0)
            token = SWAP_STORE_NUM + token;
        while(tmp){
            if(tmp->info.cr3 == cr3){
                tmp->info.valid[token] = 1;
                break;
            }
            tmp = tmp->next;
        }
        return 1;
    }
    return 0;
}
unsigned long findSwap_num(struct Cr3_queue *queue, unsigned long cr3, int token){
    struct Cr3_queue *tmp = queue;
    if(token < 0){
        token = SWAP_STORE_NUM + token;
    }
    while(tmp){
        if(token < 0)
            token = SWAP_STORE_NUM + token;
        if(tmp->info.cr3 == cr3 ){
            if(tmp->info.valid[token] == 1)
                return tmp->info.swap_num[token];
            else
                return 0; 
        }
        tmp = tmp->next;
    }
    return 0;
}
void printInfo(struct Cr3_queue *queue){
    int i;
    struct Cr3_queue *tmp;
    tmp = queue;
    while(tmp){
        printf("cr3 %lx \n", tmp->info.cr3);
        for(i=0; i<SWAP_STORE_NUM; i++){
            printf("swap_num:%lu valid:%d\n", tmp->info.swap_num[i], tmp->info.valid[i]);
        }
        tmp = tmp->next;
    }
    printf("\n");
}



void updateToken(int *token){
   *token = ((*token)+1)%5;
}

void end_process(){
    privcmd_hypercall_t hyper2 = { //stop test touched
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

int copy_into_ary(unsigned long *dst, unsigned long *src){
    int num = src[0];
    int i;
    for(i=0; i<num; i++){
        dst[i] = src[i+1];
    }
    return num;
}
int inArray(unsigned long cr3, unsigned long *cr3_list, int num){
    int i;
    for(i=0; i<num; i++){
        if(cr3 == cr3_list[i])
            return 1;
    }
    return 0;
}

int main(int argc, char *argv[])  
{ 

    unsigned long total_diff, cr3;
    unsigned long total_swap, token;
    unsigned long *swap_num, *cr3_list, *touched_cr3, *tmp;
    struct Cr3_queue q, *queue;     
    

    long i, num;
    long j;

    signal(SIGINT, end_process);
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


    queue = &q;
    queue->info.cr3 = 0;
    queue->next = NULL;
    token = 0;
    tmp = (unsigned long *)malloc(sizeof(unsigned long)*RECENT_CR3_SIZE);
    memset( tmp, 0, RECENT_CR3_SIZE);
    cr3_list = (unsigned long *)malloc(sizeof(unsigned long)*RECENT_CR3_SIZE);
    memset( cr3_list, 0, RECENT_CR3_SIZE);
    swap_num = (unsigned long *)malloc(sizeof(unsigned long)*RECENT_CR3_SIZE);
    memset( swap_num, 0, RECENT_CR3_SIZE);

    privcmd_hypercall_t hyper0 = { //init environment 
        __HYPERVISOR_change_ept_content, 
        { domID, os_type, 0, 13, 0}
    };
    ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper0);    //init environment

    flag = 0;
    int show_times, sample_times, bottleneck_flag, cr3_num;
    cr3_num = 0;
  
    printf("Start monitoring please input CTRL+C to stop monitoring\n");
    bottleneck_flag = 0;
    while( 1 ){   
        total_diff = 0;
        total_swap = 0;
        privcmd_hypercall_t hyper4 = { //stop test touched
            __HYPERVISOR_change_ept_content, 
            { domID, os_type, 0, 17, 0}
        };
        ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper4);

        memset( tmp, 0, RECENT_CR3_SIZE);
        privcmd_hypercall_t hyper5 = {  //get touched_cr3
            __HYPERVISOR_change_ept_content, 
            { domID, 0, 0, 18, tmp}
        };              
        ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper5);
        num = tmp[0];
        //printInfo(queue);
        for(i=1; i<num; i++){
            unsigned long swap1, swap2;                      
            cr3 = tmp[i];
            if(!inArray(cr3, cr3_list, cr3_num)){
                continue;
            } 
            //printf("touched cr3:%lx swap_num:%lu num:%d\n", tmp[i], findSwap_num(queue, cr3, token-1), num);
            setValid(queue, cr3, token-1);
            swap1 = findSwap_num(queue, cr3, token-1);
            swap2 = findSwap_num(queue, cr3, token-2);
//            if(swap1>swap2)
//                printf("cr3:%lx swap1:%lu swap2:%lu token:%d\n", cr3, swap1, swap2, token);
            if(swap1 > swap2 && swap2!=0 ){
                total_diff += swap1 - swap2;
                total_swap += swap1; 
            }
        }
        if(total_diff > 200){
            printf("Bottleneck need extra %lu[MB] memory\n", total_swap/256);
        }

        privcmd_hypercall_t hyper6 = {  //copy cr3 to test_cr3
            __HYPERVISOR_change_ept_content, 
            { domID, os_type, 0, 15, 0}
        };              
        ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper6);

        memset( tmp, 0, RECENT_CR3_SIZE);
        privcmd_hypercall_t hyper7 = {  //get cr3
            __HYPERVISOR_change_ept_content, 
            { domID, 0, 0, 16, tmp}
        };     
        ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper7);
        memset( cr3_list, 0, RECENT_CR3_SIZE);
        cr3_num = copy_into_ary(cr3_list, tmp);

        privcmd_hypercall_t hyper8 = {  //walk page talble
            __HYPERVISOR_change_ept_content, 
            { domID, cr3_num, 0, 7, cr3_list}
        };        
        ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper8); 


        privcmd_hypercall_t hyper1 = { //start test touched 
            __HYPERVISOR_change_ept_content, 
            { domID, os_type, 0, 20, 0}
        };
        ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper1); 


        memset( tmp, 0, RECENT_CR3_SIZE);
        privcmd_hypercall_t hyper9 = {  //get swap number
            __HYPERVISOR_change_ept_content, 
            { domID, num, 0, 19, tmp}
        };        
        ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper9);
        memset( swap_num, 0, RECENT_CR3_SIZE);
        cr3_num = copy_into_ary(swap_num, tmp);
        for(i=0; i<cr3_num; i++){
           if(swap_num[i]==INVALID_GFN){
               continue;
           }
           updateInfo(queue, cr3_list[i], swap_num[i], token); 
        }

        sleep(3);
        updateToken(&token);
    }

END:{
        end_process();
        close(fd);
    }
    return 0;

}
