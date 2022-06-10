#include <fs.h>
#include <stdint.h>
#include <stdbool.h> 
#include <stdio.h>

DirTree dt_root;


//Create filesystem tree so that we cache were things are located and don't have to search the filesystems inodes every time
void vfs_init(void){
  dt_root.name[0] = '/';
  dt_root.name[1] = '\0';
  dt_root.blockdev = 0;
  dt_root.fstype = -1;
  dt_root.vinode = pzalloc(sizeof(VInode));
  dt_root.children = NULL;

  dt_root.vinode->inode = 1;
  attachToTree(&dt_root);
}

DirTree * new_dir_tree(uint32_t inode,char name[60]){
  DirTree * dt = pzalloc(sizeof(DirTree));
  memcpy(dt->name,name,60);
  dt->vinode = pzalloc(sizeof(VInode));
  dt->vinode->inode = inode;
  return dt;
}

void add_children(DirTree *parent, DirTree * child){
  DirTreeList *entry = pzalloc(sizeof(DirTreeList));
  entry->node = child;
  child->parent = parent;
  
  entry->next = parent->children;
  parent->children = entry;
  
}

//Find element based on name
DirTree * findDirTree(char *path){
  int pos = 0;
  int path_len = strlen(path);
  DirTree dt = dt_root;
  if(path[pos] == '/'){
    pos++;
    if(path[pos] == '\0'){
      return &dt_root;
    }
  }else{
    return NULL;
  }
  DirTreeList *dtl = dt.children;
  
  while(dtl != NULL){
    int str_len = strlen(dtl->node->name);
    if(strncmp(path + pos,dtl->node->name,str_len) == 0){
      pos+=str_len;
      if(path[pos] == '/'){
        pos++;
      }
      if(pos == path_len){
        return dtl->node;
      }
      dtl = dtl->node->children;

    }else{
      dtl = dtl->next;
    }

    
  }
  return NULL;
}

//Reads an amount from an offset of a file
void read_from_file(char * path, void * buff, int file_offset, int amount){
  DirTree * dt = findDirTree(path);
  if(dt == NULL){
    printf("File not found\n");
    return;
  }
  
  switch(dt->fstype){
    case MINIX3:
      minix3_read_from_file(dt,buff,file_offset,amount);
      break;
  }
}

//Reads whole file
void * read_file(char * path, int *amount_read){
  DirTree * dt = findDirTree(path);
  if(dt == NULL){
    printf("File not found\n");
    return -1;
  }
  
  switch(dt->fstype){
    case MINIX3:
      return minix3_read_file(dt,amount_read);
      break;
  }
}


int filesize(char * path){
  DirTree * dt = findDirTree(path);
  if(dt == NULL){
    printf("File not found\n");
    return -1;
  }
  
  switch(dt->fstype){
    case MINIX3:
      return minix3_filesize(dt);
      break;
  }
}

void create_file(char * path, int type, int premissions){
  DirTree * dt = findDirTree(path);
  if(dt != NULL){
    printf("File already exist\n");
    return;
  }
  int end = strlen(path);
  int i = end;
  while(i > 0 && path[i]!='/'){

    i--;
  }
  i++;

  char * filename = pzalloc(end-i+1);
  memcpy(filename,path+i,end-i+1);
  path[i] = '\0';

  
  dt = findDirTree(path);
  if(dt == NULL){
    printf("parent path not found\n");
    return;
  }
  
  switch(dt->fstype){
    case MINIX3:
      minix3_create_file(dt, filename, type, premissions);
      break;
  }
}