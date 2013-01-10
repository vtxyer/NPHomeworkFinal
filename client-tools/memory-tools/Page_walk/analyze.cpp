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
#include <map>

#define PAGE_ACCESSED (1<<5)
#define PAGE_PRESENT  (1)
#define PAGE_PROTNONE  (1<<8)
#define PAGE_FILE  (1<<6)

using namespace std;

typedef unsigned long addr_t;
typedef unsigned long mfn_t;

xc_interface *xch;
int domID;

struct hash_table
{
	map<unsigned long, char> h;
	unsigned long cr3;
	unsigned long non2s, s2non;
};

struct guest_pagetable_walk
{
	unsigned long va;
	unsigned long l4e;            /* Guest's level 4 entry */
	unsigned long l3e;            /* Guest's level 3 entry */
	unsigned long l2e;            /* Guest's level 2 entry */
	unsigned long l1e;            /* Guest's level 1 entry (or fabrication) */
	mfn_t l4mfn;                /* MFN that the level 4 entry was in */
	mfn_t l3mfn;                /* MFN that the level 3 entry was in */
	mfn_t l2mfn;                /* MFN that the level 2 entry was in */
	mfn_t l1mfn;                /* MFN that the level 1 entry was in */
};

int get_bit(unsigned long entry, int num, int position)
{
	unsigned long mask = 0;
	int i;
	for(i=0; i<num; i++){
		mask<<=1;
		mask+=1;
	}
	return (entry&(mask<<position));
}
int page_size_flag (uint64_t entry){
	return get_bit(entry, 1, 7);
}
int entry_present(uint64_t entry){
	return get_bit(entry, 1, 0);
}


int compare_swap(struct hash_table *table, struct guest_pagetable_walk *gw, unsigned long offset, char bit)
{
	unsigned long vkey;
	char val;
	unsigned long entry_size = 8;
	int ret;
	map<unsigned long, char>::iterator it;

	vkey = gw->va;
	it = table->h.find(vkey);

	if(it == table->h.end()){
		table->h.insert(map<unsigned long, char>::value_type(vkey, bit));
		ret = 0;
	}
	else{		
		//swap off last and swap on this time
		val = table->h[vkey];
		if( val==0 && bit==1 ){	
			fprintf(stderr, "pte:%lx non2s va:%lx\n", vkey, gw->va);
			ret = 1;
		}
		else if(val==1 && bit==0){
			fprintf(stderr, "pte:%lx s2non va:%lx\n", vkey, gw->va);
			table->s2non++;
			ret = 0;
		}
		if(val!=bit){
			table->h[vkey] = bit; 
		}
	}
	return ret;
}

void* map_page(unsigned long pa_base, int level, struct guest_pagetable_walk *gw)
{
	switch(level){
		case 1:
			gw->l1mfn = pa_base;
			break;
		case 2:
			gw->l2mfn = pa_base;
			break;
		case 3:
			gw->l3mfn = pa_base;
			break;
		case 4:
			gw->l4mfn = pa_base;
			break;
	}
	pa_base >>= 12;
	return xc_map_foreign_range( xch, domID, XC_PAGE_SIZE, PROT_READ, pa_base);
}

unsigned long get_vaddr(unsigned long l1offset, unsigned long l2offset, unsigned long l3offset, unsigned long l4offset)
{
	unsigned long va = 0, tmp;

	tmp = 0;
	tmp |= (l1offset<<12);
	va |= tmp;
	tmp = 0;
	tmp |= (l2offset<<21);
	va |= tmp;tmp = 0;
	tmp |= (l3offset<<30);
	va |= tmp;tmp = 0;
	tmp |= (l4offset<<39);
	va |= tmp;

	return va;
}
int entry_valid(unsigned long entry)
{
	int flag = entry & 0xfff;
	uint32_t gflags, mflags, iflags, rc = 0;

	if( (entry&1) == 0){
		return 0; //invalid
	}
	else
		return 1; //valid
}
int pte_entry_valid(unsigned long entry)
{
	return entry_present(entry);
}

unsigned long page_walk_ia32e(addr_t dtb, int os_type, struct hash_table *table)
{
	unsigned long count=0, total=0;	
	unsigned long *l1p, *l2p, *l3p, *l4p;
	struct guest_pagetable_walk gw;
	uint64_t pml4e = 0, pdpte = 0, pde = 0, pte = 0;
	unsigned long l1offset, l2offset, l3offset, l4offset;
	int l1num, l2num, l3num, l4num;
	int flag;

	table->s2non = table->non2s = 0;
	l1num = 512;
	l2num = 512;
	l3num = 512;
//	l4num = 511;
	l4num = 1;

	l4p = (unsigned long *)map_page(dtb, 4, &gw);
	if(l4p == NULL){
		printf("cr3 map error\n");
		return -1;
	}
	for(l4offset=0; l4offset<l4num; l4offset++)
	{
		gw.l4e = l4p[l4offset];
		if( !entry_valid(gw.l4e)){
			continue;
		}
		l3p = (unsigned long *)map_page(gw.l4e, 3, &gw);
		if(l3p == NULL){
			continue;
		}
		for(l3offset=0; l3offset<l3num; l3offset++)
		{
			gw.l3e = l3p[l3offset];
			if( !entry_valid(gw.l3e)){
				continue;
			}
			l2p = (unsigned long *)map_page(gw.l3e, 2, &gw);
			if(l2p == NULL){
				continue;
			}
			for(l2offset=0; l2offset<l2num; l2offset++)
			{
				gw.l2e = l2p[l2offset];
				if( !entry_valid(gw.l2e)){
					continue;
				}
				if(page_size_flag(gw.l2e)) //map 2MB page
				{
					continue;
				}
				l1p = (unsigned long *)map_page(gw.l2e, 1, &gw);
				if(l1p == NULL){
					continue;
				}
				for(l1offset=0; l1offset<l1num; l1offset++)
				{
					gw.l1e = l1p[l1offset];
					gw.va = get_vaddr(l1offset, l2offset, l3offset, l4offset);
					
					total++;
					if( !pte_entry_valid(gw.l1e))
					{
						flag = gw.l1e & 0xfff;
                        count++;
						int ret;
						ret = compare_swap(table, &gw, l1offset, 1);
						if(ret==1)
						{
							table->non2s++;	
						}
						else if(ret == -1)
							printf("hash alloc error\n");
/*						if(os_type == 0) //linux
						{
							if( ( !( flag & (PAGE_PRESENT)))
									&& (gw.l1e != 0) 
									&&  ( !(flag & PAGE_FILE) ) 
							  )
							{
								int ret;
								ret = compare_swap(table, &gw, l1offset, 1);
								if(ret==1)
								{
									table->non2s++;	
								}
								else if(ret == -1)
									printf("hash alloc error\n");
								count++;									
							}
						}
						else if(os_type == 1) //windows
						{
                            if(  ((flag & PAGE_PRESENT)==0 )  
                                    && ( gw.l1e!= 0)
                                    && ( (flag & ((0x1f)<<5)) != 0 )
                                    )
                            {
//                                if( (flag & ((0x3<<10))) == 0){ //trasition or swap_hash
                                    count++;
									int ret;
									ret = compare_swap(table, &gw, l1offset, 1);
									if(ret==1)
									{
										table->non2s++;	
									}
									else if(ret == -1)
										printf("hash alloc error\n");
//                                }
							}
						}*/
					}
				 	else
					{
						int ret;
						ret = compare_swap(table, &gw, l1offset, 0);
						if(ret == -1)
							printf("hash alloc error\n");
					}
				}
			}
		}
	}

	printf("total: %lu\n", total);
	return count;
}




int main(int argc, char *argv[])  
{ 
	int fd, ret, i;  
	unsigned long cr3, value, os_type;
	unsigned long offset = 0;
	struct hash_table global_hash;

	
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


	while(1){
		printf("input cr3: ");
		scanf("%lx", &cr3);
		value = page_walk_ia32e(cr3, 0, &global_hash);
		printf("Page walk total:%lu non->swap:%lu swap->non:%lu\n", value, global_hash.non2s, global_hash.s2non);
		fprintf(stderr, "ok\n");		
	}


	/*    privcmd_hypercall_t hyper0 = {   //show recent_cr3
		  __HYPERVISOR_change_ept_content, 
		  { atoi(argv[1]), 0, 0, 6, 0}
		  };
		  fd = open("/proc/xen/privcmd", O_RDWR);  
		  if (fd < 0) {  
		  perror("open");  
		  exit(1);  
		  }
		  ret = ioctl(fd, IOCTL_PRIVCMD_HYPERCALL, &hyper0); */


	return 0;
}
