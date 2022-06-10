#include <stdio.h>
#include <csr.h>
#include <page.h>
#include <mmu.h>
#include <printf.h>
#include <util.h>

#define MIN_ALLOC_NODE 16

typedef struct Flist {
   int size;
   struct Flist *flink;
} *Flist;

void* malloc_head = NULL;
uint64_t kernel_heap_v_addr = 0x120000000UL;
extern PageTable *kernel_mmu_table;

//Calculate the size that we actually need to allocate to be aligned and for bookkeeping bytes
int realSize(uint64_t size){
  uint64_t remainder = size%8;
  if(remainder == 0){
    return size+8;
  }
  uint64_t quotient = size/8;
  return ((quotient+1)*8)+8;
}


void init_kernel_heap(void){
    int i;
    Flist p, v;
    p = page_phalloc();
    mmu_map(kernel_mmu_table, kernel_heap_v_addr, (unsigned long)p, (PB_READ | PB_WRITE));
    sfence_asid1(KERNEL_ASID);
    
    malloc_head = kernel_heap_v_addr;
    Flist f = (Flist *)kernel_heap_v_addr;
    f->size = PAGE_SIZE;
    f->flink = NULL;
    kernel_heap_v_addr += PAGE_SIZE;
}

void free_list_append(Flist src, Flist new){
    new->flink = src->flink;
    src->flink = new;
}


void* pmalloc(uint64_t size){
    if(malloc_head == NULL){
        init_kernel_heap();
    }
    uint64_t rsize = realSize(size);
    
    Flist f = malloc_head;
    Flist p;
    Flist back;
    Flist temp;
    uint64_t pos = -1;

    do{ //Looks in freelist for open slots

        if(f->size >= rsize+16){
            pos = f;

            break;
        }
        back = f;
        f=f->flink;
    }while(f!=NULL);

    
    if(pos == -1){ //There isn't enough mem so get more 
        f = kernel_heap_v_addr;
        
        pos = f;
        int numPages = (rsize / PAGE_SIZE) + 1;
        Flist end = back;
        
        p = page_nphalloc(numPages);
        if(p == NULL){
            printf("Out of pages\n");
            return NULL; //Out of pages
        }
        //kernel_heap_v_addr += PAGE_SIZE;
        mmu_map_multiple(kernel_mmu_table, kernel_heap_v_addr, (unsigned long)p, numPages*PAGE_SIZE, PB_READ | PB_WRITE);//PCIe
        sfence_asid1(KERNEL_ASID);

        Flist temp = kernel_heap_v_addr;
        temp->size = numPages*PAGE_SIZE;
        free_list_append(end,temp);
        end = temp;

        kernel_heap_v_addr += numPages*PAGE_SIZE;
        
        phy_coalesce();
        
        f = malloc_head;
        do{ //Relook through list because coalescing can make things funky
            if(f->size >= rsize+16){
                pos = f;
                break;
            }
            back = f;
            f=f->flink;
        }while(f!=NULL);
    }


    void* returnAddr;
    if((f->size - rsize) < MIN_ALLOC_NODE){ //Can't split it any more so give the whole thing away
        if(returnAddr == malloc_head){
            Flist tmp = returnAddr;
            malloc_head = tmp->flink;
        }
        back->flink = f->flink;
        returnAddr = pos;
    }else{
        returnAddr = pos+(f->size - rsize);
        Flist tmp = returnAddr;
        tmp->size = rsize;
        f->size -= rsize;
    }
    return returnAddr+8;
}

void pfree(void * location){
    return;
    if(location == NULL){
        return 0;
    }

    Flist f = malloc_head;
    Flist back;
    location -= 8;

    if(location < f){
        ((Flist)location)->flink = malloc_head;
        malloc_head = location;
        phy_coalesce();
        return;
    }
    do{
        if(location < f){
            break;
        }
        back = f;
        f=f->flink;
    }while(f!=NULL);
    free_list_append(back,location);

    phy_coalesce();
}

void* pzalloc(unsigned int size){
  void* pos = pmalloc(size);
  memset(pos,0,size);
  return pos;
}


void* prealloc(void* ptr,unsigned int size){
  int pos = pmalloc(size);
  if((unsigned int)(ptr-8) > size){
    memcpy(pos,ptr,size);
  }else{
    memcpy(pos,ptr,(unsigned int)(ptr-8));
  }
  pfree(ptr);
  return pos;
}

void phy_coalesce(void){
    Flist f = malloc_head;
    do{
        if( (mmu_translate(kernel_mmu_table, (uint64_t)f) + f->size) == (mmu_translate(kernel_mmu_table, (uint64_t)(f->flink))) ){
            f->size += f->flink->size;
            f->flink = f->flink->flink;
        }else{
            f=f->flink;
        }
        
    }while(f!=NULL);
}