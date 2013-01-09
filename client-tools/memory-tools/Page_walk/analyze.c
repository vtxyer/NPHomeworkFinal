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
#include "hashtab.h"
#include <xs.h>
#include <xen/hvm/save.h>

#define DOMAIN "Web"
#define PID 2

#define mfn_t unsigned long
#define PAGE_ACCESSED (1<<5)
#define PAGE_PRESENT  (1)
#define PAGE_PROTNONE  (1<<8)
#define PAGE_FILE  (1<<6)
typedef xc_interface* libvmi_xenctrl_handle_t;

xc_interface *xch;
int domID;

struct hash_table
{
	struct hashtab *h;
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
typedef struct xen_instance{
	libvmi_xenctrl_handle_t xchandle; /**< handle to xenctrl library (libxc) */
	unsigned long domainid; /**< domid that we are accessing */
	int xen_version;        /**< version of Xen libxa is running on */
	int hvm;                /**< nonzero if HVM */
	xc_dominfo_t info;      /**< libxc info: domid, ssidref, stats, etc */
	uint8_t addr_width;     /**< guest's address width in bytes: 4 or 8 */
	struct xs_handle *xshandle;  /**< handle to xenstore daemon */
	char *name;
} xen_instance_t;

int page_size_flag (uint64_t entry){
	return (entry&(1<<7));
}
int entry_present(uint64_t entry){
	return (entry&1);
}
int compare_swap(struct hashtab *h, struct guest_pagetable_walk *gw, unsigned long offset, char bit)
{
	unsigned long vkey;
	unsigned long *key;
	char *val;
	unsigned long entry_size = 8;
	int ret;

	//vkey = gw->l1mfn + offset*entry_size;
//	vkey = gw->l2e + offset*entry_size;
	vkey = gw->va;
	key = &vkey;
	val = hashtab_search(h, key);

	if(val == NULL){
		key = malloc(sizeof(unsigned long));
		val = malloc(sizeof(char));
		if(key==NULL || val==NULL){
			return -1;
		}
		*key = vkey;
		*val = bit;
		ret = hashtab_insert(h, key, val);
		if(ret!=0)
			printf("<VT>hash table insert error\n");
		ret = 0;
		return ret;
	}
	else{	
		//swap off last and swap on this time
		if( *val==0 && bit==1 ){		
			*val = 1;
			ret = 1;
			return ret;
		}
		if(*val!=bit){
			*val = bit;
		}
	}
	return 0;
}

void* map_page(vmi_instance_t vmi, unsigned long pa_base, int level, struct guest_pagetable_walk *gw)
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

unsigned long page_walk_ia32e(vmi_instance_t vmi, addr_t dtb, int os_type, struct hash_table *table)
{
	unsigned long count=0;	
	unsigned long *l1p, *l2p, *l3p, *l4p;
	struct guest_pagetable_walk gw;
	uint64_t pml4e = 0, pdpte = 0, pde = 0, pte = 0;
	unsigned long l1offset, l2offset, l3offset, l4offset;
	int l1num, l2num, l3num, l4num;
	int flag;
	struct hashtab *h;

	h = table->h;
	table->s2non = table->non2s = 0;
	l1num = 512;
	l2num = 512;
	l3num = 512;
	l4num = 511;

	l4p = map_page(vmi, dtb, 4, &gw);
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
		l3p = map_page(vmi, gw.l4e, 3, &gw);
		if(l3p == NULL){
			continue;
		}
		for(l3offset=0; l3offset<l3num; l3offset++)
		{
			gw.l3e = l3p[l3offset];
			if( !entry_valid(gw.l3e)){
				continue;
			}
			l2p = map_page(vmi, gw.l3e, 2, &gw);
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
				l1p = map_page(vmi, gw.l2e, 1, &gw);
				if(l1p == NULL){
					continue;
				}
				for(l1offset=0; l1offset<l1num; l1offset++)
				{
					gw.l1e = l1p[l1offset];
					if( !pte_entry_valid(gw.l1e)){
						gw.va =   (l1offset<<39) | (l3offset<<30) |  (l2offset<<21) | (l1offset<<12);
						flag = gw.l1e & 0xfff;
						if(os_type == 0) //linux
						{
							if( ( !( flag & (PAGE_PRESENT)))
									&& (gw.l1e != 0) 
									&&  ( !(flag & PAGE_FILE) ) 
							  )
							{
								int ret;
								ret = compare_swap(h, &gw, l1offset, 1);
								if(ret==1)
								{
									table->non2s++;	
								}
								else if(ret == -1)
									printf("hash alloc error\n");
								count++;									
							}
							else
							{
							}
						}
						else if(os_type == 1) //windows
						{
						}
					}
				 	else
					{
						int ret;
						ret = compare_swap(h, &gw, l1offset, 0);
						if(ret==1)
						{
							table->s2non++;	
						}
						else if(ret == -1)
							printf("hash alloc error\n");
					}
				}
//				free(l1p);
			}
//			free(l2p);
		}
//		free(l3p);
	}
//	if(l4p!=NULL)
//		free(l4p);

	return count;
}


static unsigned int swap_hash_val(struct hashtab *h, const void *key)
{
	const unsigned long *vkey;
	unsigned long mask = (h->size)-1;
	vkey = key;
	return  ( (*vkey)&mask );
}
static int swap_hash_cmp(struct hashtab *h, const void *key1, const void *key2)
{
	const unsigned long *vkey1, *vkey2;
	vkey1 = key1;
	vkey2 = key2;

	if(*vkey1 != *vkey2){
		return 1;
	}
	else{
		return 0;
	}
}
int hash_init(struct hash_table *h)
{
	unsigned int size;

	size = -1;
	size >>= 10;

	printf("size %lu\n", (size*sizeof(struct hashtab_node))/(1024*1024) );

	h->h = hashtab_create(swap_hash_val, swap_hash_cmp, size);
	if(!h)
		return -1;
	else 
		return 1;
}

int main(int argc, char *argv[])  
{ 
	int fd, ret, i;  
	unsigned long cr3, value;
	vmi_instance_t vmi;
	unsigned long offset = 0;
	struct hash_table global_hash;
	struct hashtab_info info;


	xch =  xc_interface_open(0,0,0);
	if(xch == NULL)
		printf("xch init error\n");
	domID = atoi(argv[1]);

	//	cr3 = (addr_t)strtoul(argv[1], NULL, 16);
	if (vmi_init(&vmi, VMI_AUTO | VMI_INIT_COMPLETE, DOMAIN) == VMI_FAILURE){
		printf("Failed to init LibVMI library.\n");
		return -1;
	}

	ret = hash_init(&global_hash);
	if(ret==-1){
		printf("hash init error\n");
		exit(1);
	}


	while(1){
		scanf("%lx", &cr3);
		value = page_walk_ia32e(vmi, cr3, 0, &global_hash);
		printf("Page walk total:%lu non->swap:%lu swap->non:%lu\n", value, global_hash.non2s, global_hash.s2non);
		hashtab_stat(global_hash.h, &info);
		printf("slot used:%d max:%d\n", info.slots_used, info.max_chain_len);
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
