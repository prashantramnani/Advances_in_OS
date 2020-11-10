#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    for(int i=0;i<128;++i) {
        inodes[i].valid = 0;
    }
}

void print_inode_info(disk* diskptr, super_block* sb) {
    for(int i=0;i<sb->inode_blocks;++i) {
        inode* x = (inode *)malloc(128*sizeof(inode));
        read_block(diskptr, sb->inode_block_idx + i, (void *)x);
        for(int j=0;j<128;++j) {
            printf("READING: %d\n", x[j].valid);    
        }
    }
}

int format(disk *diskptr) {
    
    // Check validity of diskptr
    if(diskptr==NULL)
        return -1;

    // initialise super block
    super_block* sb = (super_block *)malloc(sizeof(super_block));
    init_superblock(diskptr, sb);

    // write super block to disk
    char* C = (char *)calloc(BLOCKSIZE, sizeof(char));
    memcpy(C, sb, sizeof(super_block));
    write_block(diskptr, 0, (void *)C);    
    free(C);

    // initialize inode
    inode* inodes = (inode *)malloc(128 * sizeof(inode));
    init_inode_block(sb, inodes);
    
    // write inode to disk
    for(int i=0;i<sb->inode_blocks;++i) {
        if(write_block(diskptr, sb->inode_block_idx + i, (void *)inodes)!=0)
            return -1;    
    }
    
    // initialise bitmap
    char* C2 = (char *)calloc(BLOCKSIZE, sizeof(char));
    // initialise inode bitmap
    for(int i=1;i<sb->data_block_bitmap_idx;i++){
        if(write_block(diskptr, i, (void *)C2)!=0)
            return -1;
    }
    // initialise datablock bitmap
    for(int i=sb->data_block_bitmap_idx;i<sb->inode_block_idx;i++){
        if(write_block(diskptr, i, (void *)C2)!=0)
            return -1;  
    }
    free(C2);
    
    return 0;
}

int _set_kthbit(disk *diskptr, int bitmap_block_idx, int k){
    int* C = (int *)calloc(BLOCKSIZE, sizeof(char));
    if(read_block(diskptr, i, (void *)C)!=0){
        return -1;
    }
    C[(k/32)] |= (1 << (k%32));
    if(write_block(diskptr, i, (void *)C)!=0){
        return -1;
    }
    free(C2);
    return 0;
}

int _reset_kthbit(disk *diskptr, int bitmap_block_idx, int k){
    int* C = (int *)calloc(BLOCKSIZE, sizeof(char));
    if(read_block(diskptr, i, (void *)C)!=0){
        return -1;
    }
    C[(k/32)] &= ~(1 << (k%32));
    if(write_block(diskptr, i, (void *)C)!=0){
        return -1;
    }
    free(C2);
    return 0;
}


int mount(disk *diskptr);

int create_file();

int remove_file(int inumber);

int stat(int inumber);

int read_i(int inumber, char *data, int length, int offset);

int write_i(int inumber, char *data, int length, int offset);