#include <map>
using namespace std;

#define PAGE_ACCESSED (1<<5)
#define PAGE_PRESENT  (1)
#define PAGE_PROTNONE  (1<<8)
#define PAGE_FILE  (1<<6)

#define CHANGE_LIMIT 1
#define MAX_ROUND_INTERVAL 10
#define ADDR_MASK 0x0000ffffffffffff

xc_interface *xch;
int domID;

typedef unsigned long addr_t;
typedef unsigned long mfn_t;
typedef char byte;
typedef map<unsigned long, byte> HASHMAP;
typedef map<unsigned long, struct hash_table> DATAMAP;
typedef map<unsigned long, byte> SYSTEM_MAP;


struct hash_table
{
	map<unsigned long, byte> h; //bit 0=>valid_bit, 1~8 => counter
	unsigned long cr3;
	unsigned long non2s, s2non, count;
	unsigned long change_page, total_valid_pages;
	unsigned long activity_page[2];//0->invalid, 1->valid
	unsigned int round;
	byte check;
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

SYSTEM_MAP system_map_wks;
SYSTEM_MAP system_map_swap;


//1~7 bits represent change number
int add_change_number(byte &value)
{
	byte number;
	number = (value>>1) & 0x7f;
	if(number<0x7f)
	{
		number += 1;
		number <<= 1;
		number |= (value&1);
		value = number;
	}
}
byte get_change_number(byte value)
{
	byte number;
	number = (value>>1) & 0x7f;
	return number;
} 
int get_bit(unsigned long entry, int num, int position)
{
	unsigned long mask = 0;
	int i, value;
	for(i=0; i<num; i++){
		mask<<=1;
		mask+=1;
	}
	value = (entry&(mask<<position));
	value >>=position;
	return value;
}
int page_size_flag (uint64_t entry){
	return get_bit(entry, 1, 7);
}
int entry_present(uint64_t entry){
	return get_bit(entry, 1, 0);
}
int get_access_bit(uint64_t entry){
	return get_bit(entry, 1, 5);
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

//	if( (entry&1) == 0 || (get_access_bit(entry) == 0)){
	if( (entry&1) == 0 ){
		return 0; //invalid
	}
	else
		return 1; //valid
}
int pte_entry_valid(unsigned long entry)
{
	return entry_present(entry);
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
int check_cr3(DATAMAP &list, unsigned long cr3)
{
	DATAMAP::iterator it;
	it = list.find(cr3);
	if(it==list.end()){		
		return 0;
	}
	else{
		return 1;
	}

}
unsigned long check_cr3_list(DATAMAP &list, unsigned long *cr3_list, int list_size)
{
	unsigned long cr3, total_change_page;	
	int i, ret;

	total_change_page = 0;
	for(i=0; i<list_size; i++){
		cr3 = cr3_list[i];
		ret = check_cr3(list, cr3);
		if(ret == 0){
			struct hash_table tmp;
			tmp.cr3 = cr3;
			tmp.change_page = 0;
			list.insert(DATAMAP::value_type(cr3, tmp));
		}
		else{
			struct hash_table &h = list[cr3];
			h.check = 1;
			total_change_page += h.change_page;	
		}
	}

	return total_change_page;
}

/*
 * valid_bit=0 => in swap
 * valid_bit=1 => is valid
 * */
int compare_swap(struct hash_table *table, struct guest_pagetable_walk *gw, unsigned long offset, char valid_bit)
{
	unsigned long vkey, paddr;
	char val, tmp;
	unsigned long entry_size = 8;
	int ret = 0;
	map<unsigned long, char>::iterator it;
	SYSTEM_MAP *system_map;

	vkey = gw->va;
	/*!!!!!! NOTE !!!!!!!
	 * I assume bit 13~48 also represent swap file offset
	 * */
	paddr = ((gw->l1e) & ADDR_MASK)>>12;
	if(valid_bit == 0){
		system_map = &system_map_swap;
	}
	else{
		system_map = &system_map_wks;
	}


	/*insert into each process map*/
	it = table->h.find(vkey);
	if(it == table->h.end()){
		table->h.insert(map<unsigned long, char>::value_type(vkey, valid_bit));
		ret = 0;
	}
	else{	
		char &val_ref = table->h[vkey];	
		val = val_ref&1;
		//non-swap to swap bit
		if( val==1 && valid_bit==0 ){	
//			fprintf(stderr, "bit:%x non2s va:%lx\n", val_ref, gw->va);
			add_change_number(val_ref);
			tmp = get_change_number(val_ref);			
			if(tmp >= CHANGE_LIMIT){
				(table->change_page)++;
			}
			ret = 1;
			(table->non2s)++;	

		}
		//swap to non-swap bit
		else if(val==0 && valid_bit==1){
//			fprintf(stderr, "bit:%x s2non va:%lx\n", val_ref, gw->va);
			add_change_number(val_ref);			
			(table->s2non)++;
			ret = 0;		
		}

		if((get_change_number(val_ref)>=CHANGE_LIMIT)){
			if(valid_bit==0)
				(table->activity_page)[0]++;
			else
				(table->activity_page)[1]++;

			/*check if paddr already stored in system_map*/
			/*if(system_map->count(paddr) > 0){
				byte *paddr_times = &(system_map->at(paddr));
				if((*paddr_times) < 0xff)
					*paddr_times += 1;
				(table->activity_page)[valid_bit]++;
			}
			else{
				system_map->insert(pair<unsigned long, byte>(paddr, 1));
			}*/
		}

		if(val!=valid_bit){
			val_ref &= 0xfe;
			val_ref |= valid_bit; 
		}
	}
	return ret;
}



unsigned long page_walk_ia32e(addr_t dtb, int os_type, struct hash_table *table)
{
	unsigned long count=0, total=0;	
	unsigned long *l1p, *l2p, *l3p, *l4p;
	struct guest_pagetable_walk gw;
	uint64_t pml4e = 0, pdpte = 0, pde = 0, pte = 0, access=0;
	unsigned long l1offset, l2offset, l3offset, l4offset;
	int l1num, l2num, l3num, l4num;
	int flag;
	unsigned int hugepage_counter = 0;

	table->s2non = table->non2s = table->count = table->change_page = table->activity_page[0] = table->activity_page[1] =
	table->total_valid_pages = 0;
	l1num = 512;
	l2num = 512;
	l3num = 512;
	l4num = 512;
//	l4num = 511;
//	l4num = 1;

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
			if(page_size_flag(gw.l3e)) //1GB huge page
			{
//				hugepage_counter++;
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
				if(page_size_flag(gw.l2e)) //2MB huge page
				{
					total += 512;
					hugepage_counter++;
					continue;
				}
				l1p = (unsigned long *)map_page(gw.l2e, 1, &gw);
				if(l1p == NULL){
					continue;
				}
				for(l1offset=0; l1offset<l1num; l1offset++)
				{
					total++;
					gw.l1e = l1p[l1offset];
					gw.va = get_vaddr(l1offset, l2offset, l3offset, l4offset);

					if(gw.va == 0)
						continue;

					if( !pte_entry_valid(gw.l1e))
					{
						if(get_access_bit(gw.l1e)==1){
							access++;
//							continue;
						}

						flag = gw.l1e & 0xfff;
						count++;
						int ret;
						ret = compare_swap(table, &gw, l1offset, 0);
						/*						if(os_type == 0) //linux
												{
												if( ( !( flag & (PAGE_PRESENT)))
												&& (gw.l1e != 0) 
												&&  ( !(flag & PAGE_FILE) ) 
												)
												{
												int ret;
												ret = compare_swap(table, &gw, l1offset, 0);
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
						ret = compare_swap(table, &gw, l1offset, 0);
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
						(table->total_valid_pages)++;
						ret = compare_swap(table, &gw, l1offset, 1);
					}
				}	 
				munmap(l1p, XC_PAGE_SIZE);
			}	
			munmap(l2p, XC_PAGE_SIZE);
		}	
		munmap(l3p, XC_PAGE_SIZE);
	}	
	munmap(l4p, XC_PAGE_SIZE);

	printf("###All_pages:%lu count:%lu[M] non2s:%lu s2non:%lu access:%lu hugepage_counter:%u ###\n", total, count/256, table->non2s, table->s2non, access, hugepage_counter);
	table->count = count;
	return count;
}


int walk_cr3_list(DATAMAP &list, unsigned long *cr3_list, int list_size, int os_type, unsigned int round)
{
	unsigned long cr3;
	for(int i=0; i<list_size; i++)
	{
		cr3 = cr3_list[i];
		//If there are no struct for this cr3, map will automated assigned new struct
		struct hash_table &h = list[cr3];
		h.cr3 = cr3;
		h.check = 0;
		h.round = round;
		page_walk_ia32e(cr3, os_type, &h);
	}
}
int retrieve_list(DATAMAP &list, unsigned int round)
{
	DATAMAP::iterator it = list.begin();
	int retrieve_cr3_number = 0, interval;
	unsigned int max_int = -1;
	while(it != list.end())
	{
		struct hash_table &h = it->second;
		if(round > h.round){
			interval = round - h.round;
		}
		else{
			interval = max_int - h.round + round;	
		}

		if(interval >= MAX_ROUND_INTERVAL){
			DATAMAP::iterator erase_node = it;
			it++;
			list.erase(erase_node);
			retrieve_cr3_number++;
		}
		else{
			it++;
		}
	}

	return retrieve_cr3_number;
}
unsigned long calculate_all_page(DATAMAP &list, int os_type, unsigned long *result)
{
	DATAMAP::iterator it = list.begin();
	unsigned long check_cr3_num = 0;

	result[0] = result[1] = result[2] = 0;
	
	while(it != list.end())
	{
		check_cr3_num++;
		struct hash_table &h = it->second;
		unsigned long cr3 = it->first;
		if(h.check == 1){
			result[0] += h.activity_page[0];
			result[1] += h.activity_page[1];
			result[2] += h.total_valid_pages;
		}
		it++;
	}

	return check_cr3_num;
}
