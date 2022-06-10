#include <elf.h>
#include <printf.h>
#include <block.h>
#include <virtio.h>
#include <malloc.h>
#include <page.h>
#include <process.h>
#include <mmu.h>


//Reads an buffer than contains a elf files and loads it into a process
bool readElf(Process * p, void * elf_buffer){
    bool first_pt_load = false;
    
    uint64_t start  = 0;
    uint64_t end  = 0;

    Elf64_Ehdr *elf_header = elf_buffer;
    uint64_t *magic = pzalloc(sizeof(uint64_t));
    *magic = 0x7f454c46;
    
    if(memcmp(elf_header->e_ident,&magic) != 0){
        printf("Magic is bad\n");
        return false;
    }
    if(elf_header->e_type != ET_EXEC){ //If it isn't an executable
        printf("Elf not executable %d\n", elf_header->e_type);
        return false;
    }
    if(elf_header->e_machine != EM_RISCV){
        printf("Elf not made for this machine\n");
        return false;
    }

    Elf64_Phdr * program_header = (Elf64_Phdr *)(elf_buffer + elf_header->e_phoff);


    for(int i = 0; i < elf_header->e_phnum; i++){
        if(program_header[i].p_type == PT_LOAD){
            uint64_t vAdder = program_header[i].p_vaddr;
            uint64_t size = program_header[i].p_memsz;
            if(first_pt_load == false){
                start = vAdder;
                end = vAdder;
                first_pt_load = true;
            }else if(end < (vAdder + size)){
                end = (vAdder + size);
            }else if(start > vAdder){
                start = vAdder;
            }
        }else{
            
        }
    }

    if(first_pt_load == false){
        printf("No load sections found\n");
        return false;
    }

    uint64_t startpage = start/PAGE_SIZE;
    uint64_t endpage = (end/PAGE_SIZE)+1;
    uint64_t num_pages = (endpage-startpage);

    //Add entry to sepc
    p->frame.sepc = elf_header->e_entry;;

    //set num pages
    p->num_image_pages = num_pages;

    //alloc image
    p->image = page_nphalloc(num_pages);

    //cpy data
    for(int i = 0; i < elf_header->e_phnum; i++){
        if(program_header[i].p_type == PT_LOAD){
            memcpy(p->image + program_header[i].p_vaddr - start, elf_buffer + program_header[i].p_offset, program_header[i].p_memsz);
        }
    }
    
    //map data
    mmu_map_multiple(p->ptable, (startpage*PAGE_SIZE), p->image,(startpage*PAGE_SIZE) + (p->num_image_pages * PAGE_SIZE), PB_USER | PB_READ | PB_WRITE | PB_EXECUTE);

    return true;
}