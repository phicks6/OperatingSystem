#pragma once
#include <stdint.h>
#include <stdbool.h> 
#include <fs.h>

#define BLOCK_SIZE   1024
#define MINIX3_MAGIC 0x4d5a

#define ZONE_INDIR 7
#define ZONE_DINDR 8
#define ZONE_TINDR 9

#define S_IFDIR  16877

typedef struct SuperBlock{
    uint32_t num_inodes;
    uint16_t pad0;
    uint16_t imap_blocks;
    uint16_t zmap_blocks;
    uint16_t first_data_zone;
    uint16_t log_zone_size;
    uint16_t pad1;
    uint32_t max_size;
    uint32_t num_zones;
    uint16_t magic;
    uint16_t pad2;
    uint16_t block_size;
    uint8_t disk_version;
}SuperBlock;

typedef struct Inode{
    uint16_t mode;
    uint16_t nlinks;
    uint16_t uid;
    uint16_t gid;
    uint32_t size;
    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;
    uint32_t zones[10];
}Inode;

#define DIR_ENTRY_NAME_SIZE 60
typedef struct DirEntry{
    uint32_t inode;
    char name[DIR_ENTRY_NAME_SIZE];
}DirEntry;

#define OFFSET(inum, iblocks, zblocks) (1024 + BLOCK_SIZE + ((inum)-1) * 64 + (iblocks * BLOCK_SIZE) + (zblocks * BLOCK_SIZE))

void printSuper(void);

void attachToTree(DirTree *dt);

void minix3_read_from_file(DirTree *dt, void * buff, int file_offset, int amount);
void * minix3_read_file(DirTree *dt, int *amount_read);
int minix3_filesize(DirTree *dt);
void minix3_create_file(DirTree *dt, char *filename, int type, int premissions);