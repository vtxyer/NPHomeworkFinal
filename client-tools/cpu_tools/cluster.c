#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <curses.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <xenctrl.h>
#include <xen/xen.h>
#include <xen/sys/privcmd.h>
#include "cluster.h"

/// Some parameter in paper
#define DECAY 0.998
#define Cm 3
#define Cl 0.8
#define BETA 0.3
#define GAP 660
#define CR3_N 100

#define Dm (Cm / (CR3_N * (1 - DECAY)))
#define Dl (Cl / (CR3_N * (1 - DECAY)))

#define NEIGH_DIST 16   //< |RSP diff| <= 16K is neighbor
#define TIME_UNIT 1000000 //< convert ns to ms (i.e. * 10^6)
#define TIME_UNIT2 1000 //< convert ns to 1 sec (i.e. * 10^8) (for rsp)
#define BUSY_SEC 1 //< the time between two HLT instruction larger than this, then the domain is busy

/// Some parameter according to VM's memory size
#define CR3_BITS 20 //< # of valid CR3 bits is 20
#define CR3_NUM (1 << 20) 

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define WIN_SKIP 0x18 //< NT kernel & System, for Win64 is 0x187000, for Win32 is 0x185000
#define DELAY_SEC 1 //< if no data in hypervisor, wait 1 sec

struct cr3_list_entry *freq_list;
struct cr3_list_entry *tail_entry;

struct cr3_table_entry cr3_table[CR3_NUM];

unsigned long last_gap_time = 0;
unsigned int next_clusterID = 1;

int continue_monitor = 1;
int hlt_detect = 0;
int error_alert = 0;
int domid;

void init()
{
    int i;
    
    freq_list = NULL;
    for (i = 0; i < CR3_NUM; i++) cr3_table[i].ptr = NULL;
}

inline double density_threshold(unsigned long tg, unsigned long nowtime)
{
    return 
        ((Cl * (1 - pow(DECAY, nowtime - tg + 1)) / (CR3_N * (1 - DECAY))));
}

void free_rsp_table(struct rsp_list_entry *rsp_table)
{
    rsp_list_entry_t *temp, *ptr;

    ptr = rsp_table;

    while (ptr != NULL)
    {
        temp = ptr;
        ptr = ptr->next;
        
        free(temp);
    }
}

void gap_cluster_rsp(struct rsp_list_entry **rsp_table)
{
    rsp_list_entry_t *current, *prevEntry_1, *prevEntry_2, *nextEntry_1, *nextEntry_2, *ptr = *rsp_table;

    while (ptr != NULL)
    {
        current = ptr;
        ptr = ptr->next;

        /// if dense, set it as a cluster center
        if ( (current->st == TRANSIT || current->st == DENSE) && current->clusterID == 0)
        {
            current->clusterID = next_clusterID;

            prevEntry_1 = current;
            prevEntry_2 = current->prev;
            while (prevEntry_2 != NULL)
            {
                if (abs(prevEntry_1->rsp - prevEntry_2->rsp) <= NEIGH_DIST && prevEntry_2->clusterID == 0)
                {
                    prevEntry_2->clusterID = next_clusterID;
                }
                else break;

                prevEntry_1 = prevEntry_2;
                prevEntry_2 = prevEntry_2->next;
            }

            nextEntry_1 = current;
            nextEntry_2 = current->next;
            while (nextEntry_2 != NULL)
            {
                if (abs(nextEntry_1->rsp - nextEntry_2->rsp) <= NEIGH_DIST && nextEntry_2->clusterID == 0)
                {
                    nextEntry_2->clusterID = next_clusterID;
                }
                else break;

                nextEntry_1 = nextEntry_2;
                nextEntry_2 = nextEntry_2->next;
            }

            ptr = nextEntry_2;
            next_clusterID++;
        }
    }
}


void gap_update_rsp_table(struct rsp_list_entry **rsp_table, unsigned long time)
{
    rsp_list_entry_t *current, *ptr = *rsp_table;

    while (ptr != NULL)
    {
        current = ptr;
        ptr = ptr->next;

        /// init the clusterID to zero
        current->clusterID = 0;

        /// not new data come in, so don't plus 1
        current->density
            = current->density * pow(DECAY, time - current->tg);

        /// if sporadic, check if it has to delete or not
        if (current->st == SPORADIC)
        {
            /// this entry need to delete
            if (current->tg < last_gap_time)
            {
                /// change prev and next entry's pointer
                if (current->prev == NULL)
                {
                    *rsp_table = current->next;
                    if (current->next != NULL) current->next->prev = NULL;
                }
                else
                {
                    current->prev->next = current->next;
                    if (current->next != NULL) current->next->prev = current->prev;
                }
               
                free(current);
                continue;
            }
        }

        if (current->density >= Dm) current->st = DENSE;
        else if (current->density < Dl)
        {
            current->st = SPARSE;

            if (current->density < density_threshold(current->tg, time))
            {
                current->st = SPORADIC;
            }
        }
        else current->st = TRANSIT;
    }
}


void gap_update_cr3_density(unsigned long timestamp)
{
    unsigned long time = timestamp / TIME_UNIT; //< from ns to ms
    cr3_list_entry_t *temp_head = NULL, *temp_tail = NULL;
    cr3_list_entry_t *current, *ptr = freq_list;

    while (ptr != NULL)
    {
        current = ptr;
        ptr = ptr->next;

        current->prev = current->next = NULL;
        /// not new data come in, so don't plus 1
        current->density
            = current->density * pow(DECAY, time - current->tg); 
        
        /// if sporadic, check if it has to delete or not
        if (current->st == SPORADIC)
        {
            /// this entry need to delete
            if (current->tg < last_gap_time) 
            {
                free_rsp_table(current->rsp_table);
                cr3_table[(current->cr3 & 0xffffffff) >> 12].ptr = NULL;
                free(current);
                continue;
            }
        }

        if (current->density >= Dm) current->st = DENSE;
        else if (current->density < Dl) 
        {
            current->st = SPARSE;
            
            if (current->density < density_threshold(current->tg, time))
            {
                current->st = SPORADIC;
            }
        }
        else current->st = TRANSIT;
       
        if (current->st == DENSE) gap_update_rsp_table(&current->rsp_table, time / TIME_UNIT2);
        gap_cluster_rsp(&current->rsp_table);
        update_cr3_list(current, &temp_head, &temp_tail);
    }
    
    freq_list = temp_head;
    tail_entry = temp_tail;
}


void update_rsp(cr3_list_entry_t *ptr, unsigned long rsp, unsigned long time)
{
    rsp_list_entry_t *rspEntry, *tempPtr;

    if ( unlikely(ptr->rsp_table == NULL) )
    {
        rspEntry = malloc( sizeof(rsp_list_entry_t) );
        if (rspEntry == NULL) 
        {
            printf("Memory allocate failed at update_rsp!");
            error_alert++;
            return;
        }
        rspEntry->rsp = rsp;
        rspEntry->tg = time;
        rspEntry->density = 1.0;
        rspEntry->st = INIT;
        rspEntry->clusterID = 0;
        rspEntry->prev = rspEntry->next = NULL;

        ptr->rsp_table = rspEntry;    
    }
    else
    {
        tempPtr = ptr->rsp_table;

        while (rsp > tempPtr->rsp)
        {
            if (tempPtr->next != NULL) tempPtr = tempPtr->next;
            else break;
        }

        if (rsp == tempPtr->rsp)
        {
            tempPtr->density = 
                tempPtr->density * pow(DECAY, time - tempPtr->tg) + 1;
            tempPtr->tg = time;
        }
        else if (rsp < tempPtr->rsp)
        {
            rspEntry = malloc( sizeof(rsp_list_entry_t) );
            if (rspEntry == NULL) 
            {
                printf("Memory allocate failed at update_rsp!");
                error_alert++;
                return;
            }
            rspEntry->rsp = rsp;
            rspEntry->tg = time;
            rspEntry->density = 1.0;
            rspEntry->clusterID = 0;
            rspEntry->st = INIT;

            /// tempPtr is the head of list
            if (tempPtr->prev == NULL)
            {
                rspEntry->prev = NULL;
                rspEntry->next = tempPtr;

                tempPtr->prev = rspEntry;

                ptr->rsp_table = rspEntry;
            }
            else
            {
                tempPtr->prev->next = rspEntry;
                rspEntry->prev = tempPtr->prev;

                rspEntry->next = tempPtr;
                tempPtr->prev = rspEntry;
            }
        }
        /// rsp > tempPtr->rsp
        else
        {
            rspEntry = malloc( sizeof(rsp_list_entry_t) );
            if (rspEntry == NULL) 
            {
                printf("Memory allocate failed at update_rsp!");
                error_alert++;
                return;
            }
            rspEntry->rsp = rsp;
            rspEntry->tg = time;
            rspEntry->density = 1.0;
            rspEntry->st = INIT;
            rspEntry->clusterID = 0;
            rspEntry->prev = tempPtr;
            rspEntry->next = tempPtr->next;
            
            if (tempPtr->next != NULL) tempPtr->next->prev = rspEntry;
            tempPtr->next = rspEntry;
        }
    }
}


void update_cr3_list(cr3_list_entry_t *ptr, cr3_list_entry_t **head, cr3_list_entry_t **tail)
{
    if ( unlikely(*head == NULL) ) 
    {
        *head = ptr;
        *tail = ptr;
    }
    else
    {
        cr3_list_entry_t *prevEntry = ptr->prev;
        cr3_list_entry_t *nextEntry = ptr->next;

        /// this entry is the head
        if (ptr == *head) return;
        /// both pointer is NULL, not exist in the list
        else if (prevEntry == NULL && nextEntry == NULL)
        {
            (*tail)->next = ptr;
            ptr->prev = *tail;
            *tail = ptr;
        }
        else
        {
            if (ptr->density <= prevEntry->density) return;
            
             
            while (ptr->density > prevEntry->density)
            {
                prevEntry = prevEntry->prev;
                if (prevEntry == NULL) break;
            }
            
            /// chain the ptr prev and next
            ptr->prev->next = ptr->next;
            /// ptr is the tail
            if (ptr->next == NULL)
            {
                *tail = ptr->prev;
            }
            else
            {
                ptr->next->prev = ptr->prev;
            }

            /// Become the new head
            if (prevEntry == NULL)
            {
                (*head)->prev = ptr;
                ptr->next = *head;
                ptr->prev = NULL;

                *head = ptr;
            }
            else
            {
                ptr->next = prevEntry->next;
                if (prevEntry->next != NULL) prevEntry->next->prev = ptr;

                prevEntry->next = ptr;
                ptr->prev = prevEntry;
            }
        }
        
    }
}

void updateData(unsigned long cr3, unsigned long rsp, unsigned long timestamp)
{
    unsigned long time = timestamp / TIME_UNIT; //< from ns to ms
    unsigned long index = (cr3 & 0xffffffff) >> 12;
    unsigned long rsp_shift = (rsp >> 12); //< conver to Kbyte
    cr3_table_entry_t *e = &cr3_table[index];
    
    /// new cr3
    if (e->ptr == NULL)
    {
        e->ptr = 
            (cr3_list_entry_t*) malloc( sizeof(cr3_list_entry_t) );

        if (e->ptr == NULL)
        {
            printf("Memory allocate failed at updateData!\n");
            error_alert++;
            return;
        }

        e->ptr->cr3 = cr3;
        e->ptr->counter = 1;
        e->ptr->density = 1.0;
        e->ptr->tg = time;
        e->ptr->prev = e->ptr->next = NULL;
        e->ptr->rsp_table = NULL;
    }
    else
    {
        e->ptr->counter += 1;
        e->ptr->density = 
            e->ptr->density * pow(DECAY, time - e->ptr->tg) + 1;
        e->ptr->tg = time;
    }

    update_cr3_list(e->ptr, &freq_list, &tail_entry);

    if (((rsp >> 63) == 0))update_rsp(e->ptr, rsp_shift, time / TIME_UNIT2);
}


void *show_result()
{
    int x, y, ch, show_sec = 1000;
    char time_ch[100];
    initscr();
    noecho();
    getmaxyx(stdscr, y, x);
    timeout(show_sec);
    while ( continue_monitor )
    {
        move(0, 0);
        printw("ITRI - CPU performance bottleneck detection\n");
        printw("\tMonitor domain id: %d\n", domid);
        printw("\tUpdate interval: %d ms\n", show_sec);
        printw("\tBusy threads: %d\n", next_clusterID - 1);
        printw("\tLack of CPU resource: ");
        if (hlt_detect) printw("Yes\n");
        else printw("No\n");
        if (error_alert != 0)
            printw("%d errors happened...\n", error_alert);
        
        move(y - 1, 0);
        addch(A_REVERSE | 'D');
        printw("elay ");
        addch(A_REVERSE | 'Q');
        printw("uit ");
        refresh();

        ch = getch();
        if (ch == 'Q' || ch == 'q') break;
        else if (ch == 'D' || ch == 'd')
        {
            move(y - 1, 0);
            clrtoeol();
            printw("Delay time(ms): ");
            echo();
            timeout(-1);
            getstr(time_ch);
            show_sec = atoi(time_ch);
            if (show_sec <= 0) show_sec = 1000;
            timeout(show_sec);
            noecho();
            clrtoeol();
        }
    }

    continue_monitor = 0;
    endwin();

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int fd, ret;
    pRecord data = malloc ( sizeof(record_t) );
    long long *hlt_time = malloc ( sizeof(long long) );
    unsigned long counter = 0;
    pthread_t gui_thread;

    if (argc != 2)
    {
        printf("%s domid\n", argv[0]);
        exit(-1);
    }
    domid = atoi(argv[1]); 
    
    privcmd_hypercall_t start_monitor = 
    {
        __HYPERVISOR_cpu_monitor,
        {0, domid, (unsigned long) data, 0, 0}
    };

    privcmd_hypercall_t stop_monitor =
    {
        __HYPERVISOR_cpu_monitor,
        {1, domid, (unsigned long) data, 0, 0}
    };

    privcmd_hypercall_t cap_data =
    {
        __HYPERVISOR_cpu_monitor,
        {2, domid, (unsigned long) data, 0, 0}
    };
    
    privcmd_hypercall_t hlt_hcall =
    {
        __HYPERVISOR_cpu_monitor,
        {3, domid, (unsigned long) hlt_time, 0, 0}
    };

    fd = open("/proc/xen/privcmd", O_RDWR);
    
    if (fd < 0)
    {
        printf("Error while opening /proc/xen/privcmd\n");
        exit(-1);
    }

    /** Start monitor guest domain */
    if (ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &start_monitor) < 0)
    {
        printf("Error while starting monitoring domain %d\n", domid);
        exit(-1);
    }

    
    ret = pthread_create(&gui_thread, NULL, show_result, NULL);
    if (ret != 0)
    {
        printf("Can't create GUI interface!\n");
        exit(-1);
    }
 
    init();

    while( continue_monitor )
    {
        if (ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &cap_data) < 0)
        {
            printf("Error while capturing domain %d data\n", domid);
            error_alert++;
        }
        
        if (data->cr3 != 0)
        {
            if ( (data->cr3 >> 16) != WIN_SKIP) updateData(data->cr3, data->rsp, data->timestamp);

            counter++;
            if (counter % GAP == 0)
            {
                next_clusterID = 1;
                gap_update_cr3_density(data->timestamp);

                last_gap_time = data->timestamp / TIME_UNIT;

                if (ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hlt_hcall) < 0)
                {
                    printf("Error while geting HLT info about domain %d\n", domid);
                    error_alert++;
                }
                
                if (*hlt_time > BUSY_SEC * 1000000000) //< convert sec to ns
                {
                    hlt_detect = 1;
                }
                else
                {
                    hlt_detect = 0;
                }
            }
        }
        else
        {
            sleep(DELAY_SEC);
        }
    }

    if (ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &stop_monitor) < 0)
    {
        printf("Error while stoping monitoring domain %d\n", domid);
        exit(-1);
    }

    pthread_join(gui_thread, NULL);
	
    printf("Stop monitoring domain %d\n", domid);

    return 0;
}
