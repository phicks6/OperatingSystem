#include <page.h>
#include <symbols.h>
#include <printf.h>
#include <util.h>
#include <stddef.h>
#include <lock.h>

#define PAGES_PER_BYTE 4
unsigned long start = sym_start(heap);
unsigned long end = sym_end(heap);
struct Mutex continousLock = {MUTEX_UNLOCKED};


//Clears the bookkeeping bytes to 0's
void page_init(void){
    long total_pages = (end-start)/PAGE_SIZE;
    //printf("We have %d pages\n",total_pages);
    long bookkeepingPages = (total_pages/(PAGE_SIZE*PAGES_PER_BYTE))+1;
    //printf("We need %d bookkeepingPages\n",bookkeepingPages);
    memset(start, 0, PAGE_SIZE*bookkeepingPages);//Clear out the bookkeeping bytes
}



//Allocates 1 page
void *page_phalloc(void){
    return page_nphalloc(1);
}

//Allocates count continuous pages
void *page_nphalloc(int count){
    mutex_spinlock(&continousLock);
    unsigned long total_pages = (end-start)/PAGE_SIZE;
    unsigned long bookkeepingPages = (total_pages/(PAGE_SIZE*PAGES_PER_BYTE))+1;
    unsigned long availablePages = total_pages - bookkeepingPages;
    unsigned long true_start = start + (PAGE_SIZE*bookkeepingPages);
    
    char *bk = start;
    long *bkAdr = &bk;
    int i = 0;
    int bitPos = 0;
    int size = 0;
    while(bk < start+(availablePages/PAGES_PER_BYTE)){ //Go through every byte
        bitPos = 0;
        while(bitPos < 8){ //go through every 2 bit pair
            char taken = ((*bk) >> (7 - bitPos)) & 1;
            char last = ((*bk) >> (6 - bitPos)) & 1;
            if(!taken){ //If the page is not taken then add 1 to the continuous pages so far
                size++;
                if(size == count){ //if size equals count then we have found enough continous pages to meet the request
                    //reverse reverse
                    *bk = *bk | (1 << (7 - bitPos)) | (1 << (6 - bitPos)); //Sets both the taken and last bit to 1
                    size--;
                    while(size){
                        bitPos-=2;
                        if(bitPos < 0){
                            bitPos = 6;
                            bk--;
                        }
                        *bk = *bk | (1 << (7 - bitPos));
                        size--;
                    }
                    
                    unsigned long pageNum = (*bkAdr-start)*4 + (bitPos/2);
                    //printf("Giving away %d pages starting at page %d\n",count,pageNum);
                    void *rtn = true_start+(pageNum*PAGE_SIZE);
                    mutex_unlock(&continousLock);
                    return memset(rtn, 0, (count*PAGE_SIZE));
                }
            }else{ //If the page is taken then reset the continous pages so far to 0
                size = 0;
            }
            bitPos+=2;
        }

        i++;
        bk++;
    }
    printf("\n\n\n\n\nVERY BAD YOU HAVE RUN OUT OF PAGES\n\n");
    return NULL;
}

void page_free(void *page){
    unsigned long total_pages = (end-start)/PAGE_SIZE;
    unsigned long bookkeepingPages = (total_pages/(PAGE_SIZE*PAGES_PER_BYTE))+1;
    unsigned long availablePages = total_pages - bookkeepingPages;
    unsigned long true_start = start + (PAGE_SIZE*bookkeepingPages);
    
    long *pageAddr = &page;
    long offset = (*pageAddr-true_start)/PAGE_SIZE;
    

    char *bk = start+(offset/4);
    int bitPos = (offset%4) * 2;

    char last = ((*bk) >> (6 - bitPos)) & 1;
    char mask;
    while(!last){//While the last bit is not one, continue clearing the taken bit
        mask = ~(1 << (7 - bitPos));
        *bk = *bk & mask;
        bitPos+=2;
        if(bitPos == 8){
            bitPos = 0;
            bk++;
        }
        last = ((*bk) >> (6 - bitPos)) & 1;
    }
    //Clear the last 2 bits
    mask = ~(1 << (7 - bitPos));
    *bk = *bk & mask;
    mask = ~(1 << (6 - bitPos));
    *bk = *bk & mask;
    return;
}

uint64_t align_up(uint64_t addr){
    //return addr;
    return addr & -PAGE_SIZE;
}
uint64_t align_down(uint64_t addr){
    //return addr;
    return (addr+(PAGE_SIZE-1)) & -PAGE_SIZE;
}