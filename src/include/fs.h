#pragma once
#include <stdint.h>
#include <stdbool.h> 

#pragma once

#define S_FMT(x)       (S_IFMT & (x))

#define S_IFMT        0170000 /* These bits determine file type.  */

/* File types.  */
#define S_IFDIR       0040000 /* Directory.  */
#define S_IFCHR       0020000 /* Character device.  */
#define S_IFBLK       0x4000 /* Block device.  */
#define S_IFREG       0100000 /* Regular file.  */
#define S_IFIFO       0010000 /* FIFO.  */
#define S_IFLNK       0120000 /* Symbolic link.  */
#define S_IFSOCK      0140000 /* Socket.  */



typedef struct VInode{
    uint16_t mode;
    uint16_t uid;
    uint16_t gid;
    uint32_t inode;
}VInode;

#define MINIX3 1

typedef struct DirTree{
    char name[256];
    uint32_t blockdev;
    uint32_t fstype;
    VInode *vinode;
    struct DirTree *parent;
    struct DirTreeList *children;
}DirTree;

typedef struct DirTreeList{
  struct DirTree *node;
  struct DirTreeList *next;
}DirTreeList;

void vfs_init(void);
DirTree * new_dir_tree(uint32_t inode,char name[60]);
void add_children(DirTree *parent, DirTree * child);
DirTree * findDirTree(char *path);

void read_from_file(char * path, void * buff, int offset, int amount);
void * read_file(char * path, int *amount_read);
int filesize(char * path);
void create_file(char * path, int type, int premissions);