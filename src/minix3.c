#include <minix3.h>
#include <stdint.h>
#include <stdbool.h> 
#include <block.h>
#include <malloc.h>
#include <util.h>
#include <fs.h>


void printSuper(void){
  void *buff = pzalloc(32);
  block_read(buff, 1024, 32);
  SuperBlock * sb = buff;
  //printf("num_inodes: %d\n",sb->num_inodes);
  //printf("imap_blocks: %d\n",sb->imap_blocks);
  //printf("zmap_blocks: %d\n",sb->zmap_blocks);
  //printf("first_data_zone: %d\n",sb->first_data_zone);
  //printf("num_zones: %d\n",sb->num_zones);
  //printf("magic: %lx\n",sb->magic);
}

//Reads a disk and populates the cached filesystem with file information.
void attachToTree(DirTree *dt){
  dt->fstype = MINIX3;
  if(strcmp(dt->name,".") == 0 || strcmp(dt->name,"..") == 0){
    
    return;
  }

  void *buff = pzalloc(32);
  void *block_buff = pzalloc(BLOCK_SIZE);
  block_read(buff, 1024, 32);
  SuperBlock * sb = buff;
  uint64_t root_offset = OFFSET(dt->vinode->inode, sb->imap_blocks, sb->zmap_blocks);
  Inode *inode = pzalloc(64);
  block_read(inode, root_offset, 64);
  dt->vinode->mode = inode->mode;
  dt->vinode->uid = inode->uid;
  dt->vinode->gid = inode->gid;
  
  char * filename = pzalloc(256);
  if(S_FMT(inode->mode) == S_IFDIR || S_FMT(inode->mode) == S_IFBLK){
    for(int i = 0; i < 7; i++){
      if(inode->zones[i] != 0){
        uint64_t off = inode->zones[i]*BLOCK_SIZE;
        block_read(block_buff, off, BLOCK_SIZE);
        DirEntry * de = block_buff;
        for(int j = (BLOCK_SIZE/64)-1; j >= 0; j--){
          if(de[j].inode != 0){
            strcpy(filename,de[j].name);
            if(strcmp(dt->name,".") == 0){
              add_children(dt,dt);
            }else if(strcmp(dt->name,"..") == 0){
              add_children(dt,dt->parent);
            }else{
              add_children(dt,new_dir_tree(de[j].inode,de[j].name));
            }
          }
        }
      }
    }

    DirTreeList *dtl = dt->children;
    while(dtl != NULL){
      attachToTree(dtl->node);
      dtl = dtl->next;
    }
  }
}

int find_zone(uint32_t *zones,int offset, int level, int *amount_passed){
  if(level == 0){
    for(int i = 0; i < 7; i++){
      if(zones[i] !=0){
        *amount_passed+=BLOCK_SIZE;
        if(*amount_passed > offset){
          return zones[i];
        }
      }
    }

    //Check indirect
    if(zones[ZONE_INDIR] !=0){
      void * block_buff = pzalloc(1024);
      uint64_t off = zones[ZONE_INDIR]*BLOCK_SIZE;
      block_read(block_buff, off, BLOCK_SIZE);
      int rtn = find_zone(block_buff,offset,1,amount_passed);
      if(rtn != -1){
        return rtn;
      }
    }

    //Check doublely indirect
    if(zones[ZONE_DINDR] !=0){
      void * block_buff = pzalloc(1024);
      uint64_t off = zones[ZONE_DINDR]*BLOCK_SIZE;
      block_read(block_buff, off, BLOCK_SIZE);
      int rtn = find_zone(block_buff,offset,2,amount_passed);
      if(rtn != -1){
        return rtn;
      }
    }

    //Check triplely indirect
    if(zones[ZONE_TINDR] !=0){
      void * block_buff = pzalloc(1024);
      uint64_t off = zones[ZONE_TINDR]*BLOCK_SIZE;
      block_read(block_buff, off, BLOCK_SIZE);
      int rtn = find_zone(block_buff,offset,2,amount_passed);
      if(rtn != -1){
        return rtn;
      }
    }

  }

  if(level == 1){
    int inodes_in_block = (BLOCK_SIZE/4);
    for(int i = 0; i < inodes_in_block; i++){
      if(zones[i] !=0){
        *amount_passed+=BLOCK_SIZE;
        if(*amount_passed > offset){
          return zones[i];
        }
      }
    }
    return -1;
  }

  if(level == 2){
    int inodes_in_block = (BLOCK_SIZE/4);
    for(int i = 0; i < inodes_in_block; i++){
        void * block_buff = pzalloc(1024);
        uint64_t off = zones[i]*BLOCK_SIZE;
        block_read(block_buff, off, BLOCK_SIZE);
        int rtn = find_zone(block_buff, offset, 1, amount_passed);
        if(rtn != -1){
          return rtn;
        }
    }
    return -1;
  }

  if(level == 3){
    int inodes_in_block = (BLOCK_SIZE/4);
    for(int i = 0; i < inodes_in_block; i++){
        void * block_buff = pzalloc(1024);
        uint64_t off = zones[i]*BLOCK_SIZE;
        block_read(block_buff, off, BLOCK_SIZE);
        int rtn = find_zone(block_buff, offset, 2, amount_passed);
        if(rtn != -1){
          return rtn;
        }
    }
    return -1;
  }


  return -1;
}

//Read from file starting at offset
void minix3_read_from_file(DirTree *dt, void * buff, int file_offset, int amount){
  if(S_FMT(dt->vinode->mode) == S_IFDIR){
    printf("Can't read dir\n");
    return;
  }

  SuperBlock * sb = pzalloc(32);
  block_read(sb, 1024, 32);

  uint64_t offset = OFFSET(dt->vinode->inode, sb->imap_blocks, sb->zmap_blocks);
  Inode *inode = pzalloc(64);
  block_read(inode, offset, 64);

  int read_offset = 0;
  while(amount > 0){
    int amount_read = 0;
    int zone = find_zone(inode->zones,file_offset,0,&amount_read);
    if(amount <= BLOCK_SIZE){
      block_read(buff+read_offset,zone*BLOCK_SIZE,amount);
      return;
    }else{
      block_read(buff+read_offset,zone*BLOCK_SIZE,BLOCK_SIZE);
      read_offset+=BLOCK_SIZE;
      file_offset+=BLOCK_SIZE;
      amount-=BLOCK_SIZE;
    }
  }
}

void * minix3_read_file(DirTree *dt, int *amount_that_was_read){
  if(S_FMT(dt->vinode->mode) == S_IFDIR){
    printf("Can't read dir\n");
    return;
  }

  SuperBlock * sb = pzalloc(32);
  block_read(sb, 1024, 32);


  
  uint64_t offset = OFFSET(dt->vinode->inode, sb->imap_blocks, sb->zmap_blocks);
  Inode *inode = pzalloc(64);
  block_read(inode, offset, 64);

  int amount = inode->size;
  int file_offset = 0;
  void *buff = pzalloc(amount);

  int read_offset = 0;
  while(amount > 0){
    int amount_read = 0;
    int zone = find_zone(inode->zones,file_offset,0,&amount_read);
    
    if(amount <= BLOCK_SIZE){
      block_read(buff+read_offset,zone*BLOCK_SIZE,amount);
      *amount_that_was_read = inode->size;
      return buff;
    }else{
      block_read(buff+read_offset,zone*BLOCK_SIZE,BLOCK_SIZE);
      read_offset+=BLOCK_SIZE;
      file_offset+=BLOCK_SIZE;
      amount-=BLOCK_SIZE;
    }
  }
}


int minix3_filesize(DirTree *dt){
  if(S_FMT(dt->vinode->mode) == S_IFDIR){
    printf("Can't read dir\n");
    return;
  }

  SuperBlock * sb = pzalloc(32);
  block_read(sb, 1024, 32);

  uint64_t offset = OFFSET(dt->vinode->inode, sb->imap_blocks, sb->zmap_blocks);
  Inode *inode = pzalloc(64);
  block_read(inode, offset, 64);
  return inode->size;
  
}


void minix3_create_file(DirTree *dt, char *filename, int type, int premissions){
  if(S_FMT(dt->vinode->mode) != S_IFDIR && S_FMT(dt->vinode->mode) != S_IFBLK){
    printf("Parent not dir or block\n");
    return;
  }

  SuperBlock * sb = pzalloc(32);
  block_read(sb, 1024, 32);
  char * buffer = pzalloc(sb->imap_blocks*BLOCK_SIZE);
  char * bits = buffer;
  block_read(buffer,1024+BLOCK_SIZE,sb->imap_blocks*BLOCK_SIZE);
  int bitPos = 0;
  bool foundSlot = false;
  int index = -1;
  while(!foundSlot){
    bitPos = 0;
    while(bitPos < 8){ 
      if( (((*bits) >> bitPos) & 1) == 0 ){
        foundSlot = true;
        index = (bits-buffer)*8 + bitPos;

        //Flip bit
        *bits = *bits | (1 << bitPos);
        //block_write(bits,1024+BLOCK_SIZE+(bits-buffer),1);

        
        break;
      }
      bitPos++;
    }
    bits++;
    
  }
  pfree(buffer);

  Inode * new_inode = pzalloc(sizeof(Inode));
  new_inode->mode = (type|premissions);
  new_inode->nlinks = 1;
  new_inode->nlinks = 1;
  int byte = OFFSET(index,sb->imap_blocks, sb->zmap_blocks);
  

  
  
  if(type != S_IFDIR){
    //block_write(new_inode,byte,sizeof(Inode));
    return;
  }

  buffer = pzalloc(sb->zmap_blocks*BLOCK_SIZE);
  bits = buffer+(sb->first_data_zone/8);
  int zonemapoffset = 1024+BLOCK_SIZE+(sb->imap_blocks*BLOCK_SIZE);
  block_read(buffer,1024+BLOCK_SIZE+(sb->imap_blocks*BLOCK_SIZE),sb->zmap_blocks*BLOCK_SIZE);


  bitPos = sb->first_data_zone%8+1;
  foundSlot = false;
  index = -1;
  while(!foundSlot){
    while(bitPos < 8){ 
      if( (((*bits) >> bitPos) & 1) == 0 ){
        foundSlot = true;
        index = (bits-buffer)*8 + bitPos;

        //Flip bit
        *bits = *bits | (1 << bitPos);
        //block_write(bits,1024+BLOCK_SIZE+(sb->imap_blocks*BLOCK_SIZE)+(bits-buffer),1);

        break;
      }
      bitPos++;
    }
    bits++;
    bitPos = 0;
  }

  new_inode->zones[0] = index;
}