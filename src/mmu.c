#include <mmu.h>
#include <page.h>
#include <lock.h>

#define PTE_PPN0_SHIFT  10
#define PTE_PPN1_SHIFT  19
#define PTE_PPN2_SHIFT  28

#define ADDR_0_SHIFT 12
#define ADDR_1_SHIFT 21
#define ADDR_2_SHIFT 30

#define READ_WRITE_EXCUTE_BITS 14

#define _9_BIT_MASK 0x1FF
#define _12_BIT_MASK 0xFFF
#define _21_BIT_MASK 0x1FFFFF
#define _26_BIT_MASK 0x3FFFFFF

PageTable *kernel_mmu_table;
Mutex kernel_mmu_table_lock;

//Reads the page table to translates a virtual address into a physical one 
uint64_t mmu_translate(PageTable *table, uint64_t vaddr){
    uint64_t vpn[3]; //Get vpns from vaddress by shifting 
    vpn[0] = ((vaddr >> ADDR_0_SHIFT) & _9_BIT_MASK);
    vpn[1] = ((vaddr >> ADDR_1_SHIFT) & _9_BIT_MASK);
    vpn[2] = ((vaddr >> ADDR_2_SHIFT) & _9_BIT_MASK);

    uint64_t entry;

    for(int i=2; i>=0; i--){
        entry = table->entries[vpn[i]]; //Use vpn of level i to get the tab entry
        if(!(entry & PB_VALID)){ //Check the 0's bit to ensure that the entry is valid
            return 0; //If not then return 0
        }else if((entry & READ_WRITE_EXCUTE_BITS)){ //if any of the rwx are set then this is a leaf
            uint64_t ppn[3];
            uint64_t rtn;
            if(i==2){
                ppn[2] = (entry >> PTE_PPN2_SHIFT) & _26_BIT_MASK;
                rtn = (ppn[2] << ADDR_2_SHIFT) | (vaddr & _26_BIT_MASK);
                return rtn;
            }else if(i==1){
                ppn[2] = (entry >> PTE_PPN2_SHIFT) & _26_BIT_MASK;
                ppn[1] = (entry >> PTE_PPN1_SHIFT) & _9_BIT_MASK;
                rtn = (ppn[2] << ADDR_2_SHIFT) | (ppn[1] << ADDR_1_SHIFT) | (vaddr & _21_BIT_MASK);
                return rtn;
            }else{
                ppn[2] = (entry >> PTE_PPN2_SHIFT) & _26_BIT_MASK;
                ppn[1] = (entry >> PTE_PPN1_SHIFT) & _9_BIT_MASK;
                ppn[0] = (entry >> PTE_PPN0_SHIFT) & _9_BIT_MASK;
                rtn = (ppn[2] << ADDR_2_SHIFT) | (ppn[1] << ADDR_1_SHIFT) | (ppn[0] << ADDR_0_SHIFT) | (vaddr & _12_BIT_MASK);
                return rtn;
            }
        }else{//If none of the rwx bits are set then this is a branch and we need to go deeper
            table = (PageTable *)((entry << 2) & ~0xFFFUL);
        }
    }
    return 0; //Just in case
}

//Maps a virtual address to a phyiscal one
bool mmu_map(PageTable *table, uint64_t vaddr, uint64_t paddr, uint64_t bits){
    uint64_t vpn[3]; //Get vpns from vaddress by shifting 
    vpn[0] = ((vaddr >> ADDR_0_SHIFT) & _9_BIT_MASK);
    vpn[1] = ((vaddr >> ADDR_1_SHIFT) & _9_BIT_MASK);
    vpn[2] = ((vaddr >> ADDR_2_SHIFT) & _9_BIT_MASK);
    uint64_t ppn[3];
    ppn[0] = ((paddr >> ADDR_0_SHIFT) & _9_BIT_MASK);
    ppn[1] = ((paddr >> ADDR_1_SHIFT) & _9_BIT_MASK);
    ppn[2] = ((paddr >> ADDR_2_SHIFT) & _26_BIT_MASK);
                
    uint64_t entry;

    mutex_spinlock(&kernel_mmu_table_lock);
    for(int i = 2; i >= 1; i--){
        entry = table->entries[vpn[i]];
        if(!(entry & PB_VALID)){ //If not valid then make a new one
            
            PageTable *new_table = page_phalloc();
            entry = (((uint64_t)new_table) >> 2) | PB_VALID; //Put new table address in entry and set it to valid
            table->entries[vpn[i]] = entry;
        }
        //Go to next layer of table
        table = (PageTable *)((entry << 2) & ~0xFFFUL);
    }

    //Always want leafs to be a level 0
    entry = (ppn[2] << PTE_PPN2_SHIFT) | (ppn[1] << PTE_PPN1_SHIFT) | (ppn[0] << PTE_PPN0_SHIFT) | bits | PB_VALID; //Store physical address in to page table, make it valid, and set the bits
    table->entries[vpn[0]] = entry;

    mutex_unlock(&kernel_mmu_table_lock);
    return true;
}

//Recursive free
void mmu_free(PageTable *table){
    uint64_t entry;
    
    for(int i = 0; i < (PAGE_SIZE / 8); i++){
        entry = table->entries[i];
        if(entry & 1){ //if its not valid we don't need to do anything
            if(!(entry & READ_WRITE_EXCUTE_BITS)){//If it is a branch
                entry = (entry << 2) & ~0xFFFUL; //Find the next level's page table address and free that one 
                mmu_free((PageTable *)entry);
            }else{ //If it is a leaf
                table->entries[i] = 0; //Clear it, notably this will make the valid bit 0
            }
        }
    }
    //Once we have cleansed the memory free the page
    page_free(table);
}


void mmu_map_multiple(PageTable *table, uint64_t start_virt, uint64_t start_phys, uint64_t num_bytes, uint64_t bits){
    for(uint64_t i=0; i < num_bytes; i+=PAGE_SIZE){
        mmu_map(table, start_virt + i, start_phys + i, bits);
    }
}
