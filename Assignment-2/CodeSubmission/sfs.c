#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include "sfs.h"

void print_superblock_info(super_block* sb) {
    printf("Super Block Info\n\n");
    printf("Magic Number: %d\n", sb->magic_number);
    printf("Number of Blocks: %d\n", sb->blocks);

    printf("Number of inode blocks: %d\n", sb->inode_blocks);
	printf("Number of inodes: %d\n", sb->inodes);
    printf("Starting index of inode bitmap: %d\n", sb->inode_bitmap_block_idx);
    printf("Starting index of inode block: %d\n", sb->inode_block_idx);

    printf("Number of data blocks: %d\n", sb->data_blocks);
    printf("Starting index of data bitmap: %d\n", sb->data_block_bitmap_idx);
    printf("Starting index of data blocks: %d\n", sb->data_block_idx);
}

void init_superblock(disk* diskptr, super_block* sb) {
    int M = diskptr->blocks - 1; 
    int I = 0.1*M; 
    int IB = (I*128) / (8*4096);
    if((I*128)%(8*4096) != 0) {
        IB ++ ;
    }
    int R = M - I - IB;
    int DBB = (R) / (8*4096);
    if((R%(8*4096)) != 0) {
        DBB++ ;
    }
    int DB = R - DBB;

    sb->magic_number = 12345;
    sb->blocks = M; 

    sb->inode_blocks = I;
    sb->inodes = I * 128;
    sb->inode_bitmap_block_idx = 1;
    sb->inode_block_idx = 1 + IB + DBB;

    sb->data_block_bitmap_idx = 1 + IB;
    sb->data_block_idx = 1 + IB + DBB + I;
    sb->data_blocks = DB;

}

void init_inode_block(super_block* sb, inode* inodes) {
    for(int i=0;i<sb->inode_blocks;++i) {
        inodes[i].valid = i;
    }
}

int format(disk *diskptr) {
    super_block* sb = (super_block *)malloc(sizeof(super_block));
    init_superblock(diskptr, sb);
    write_block(diskptr, 0, (void *)sb);    

    inode* inodes = (inode *)malloc(sb->inode_blocks * 128 * sizeof(inode));
    init_inode_block(sb, inodes);
    inode* I = inodes;
    for(int i=0;i<sb->inode_blocks;++i) {
        write_block(diskptr, sb->inode_block_idx + i, (void *)I++);    
    }
    // for(int i=0;i<sb->inode_blocks;++i) {
    //     inode* x = (inode *)malloc(sizeof(inode));
    //     read_block(diskptr, sb->inode_block_idx + i, (void *)x);
    //     printf("READING: %d\n", x->valid);    
    // }

    return 0;
}

int mount(disk *diskptr);

int create_file();

int remove_file(int inumber);

int stat(int inumber);

int read_i(int inumber, char *data, int length, int offset);

int write_i(int inumber, char *data, int length, int offset);