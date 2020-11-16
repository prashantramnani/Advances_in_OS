#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#include "sfs.h"

#define ceil(a,b) ((a+b-1)/b)

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
    if(read_block(diskptr, bitmap_block_idx, (void *)C)!=0){
        return -1;
    }
    C[(k/32)] |= (1 << (k%32));
    if(write_block(diskptr, bitmap_block_idx, (void *)C)!=0){
        return -1;
    }
    free(C);
    return 0;
}

int mount(disk *diskptr) {
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(diskptr, 0, (void *)c);
    super_block* sb = (super_block*)malloc(sizeof(super_block));
    memcpy(sb, c, sizeof(super_block));

    if(sb->magic_number == 12345) {
        global_diskptr = diskptr;
        return 0;
    }
    return -1;
}

int _reset_kthbit(disk *diskptr, int bitmap_block_idx, int k){
    int* C = (int *)calloc(BLOCKSIZE, sizeof(char));
    if(read_block(diskptr, bitmap_block_idx, (void *)C)!=0){
        return -1;
    }
    C[(k/32)] &= ~(1 << (k%32));
    if(write_block(diskptr, bitmap_block_idx, (void *)C)!=0){
        return -1;
    }
    free(C);
    return 0;
}


int mount(disk *diskptr);

int create_file() {
    if(global_diskptr == NULL) {
		return -1;
	}

    // Read Super Block
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, 0, (void *)c);
    super_block* sb = (super_block*)malloc(sizeof(super_block));
    memcpy(sb, c, sizeof(super_block));


    // Find first empty bitmap
    int bitmap_block_i=1, bits_i=0, flag=0;
    for(; bitmap_block_i<sb->data_block_bitmap_idx;bitmap_block_i++){
        int* A = (int *)calloc(BLOCKSIZE, sizeof(char));
        if(read_block(global_diskptr, bitmap_block_i, (void *)A)!=0){
            return -1;
        }
        int k=0;
        for(bits_i = 0; bits_i<BLOCKSIZE/4;bits_i++){
            for(k=0;k<32;k++){
                if(!( A[(bits_i)] & (1 << k) )){
                    // Unset Bit found
                    A[bits_i] |= (1 << k);
                    flag=1;
                    // Update the inode bitmap
                    if(write_block(global_diskptr, bitmap_block_i, (void *)A)!=0){
                        return -1;
                    }
                    break;
                }
            }
            if(flag)
                break;
        }
        if(flag){
            // Bitmap found
            // Update inode
            // 1024*32*bitmap_block_i + 32*bits_i + k
            int inode_idx = BLOCKSIZE*8*bitmap_block_i + 32*bits_i + k;
            int inode_block_idx = sb->inode_block_idx + inode_idx/128;
            int inode_i = inode_idx%128;
        
            char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
            read_block(global_diskptr, inode_block_idx, (void *)c);
            inode* __inode = (inode*)malloc(sizeof(inode));
            memcpy(__inode, c+(32*inode_i), sizeof(inode));
            __inode->size=0;
            __inode->valid=1;
            memcpy(c+(32*inode_i), __inode, sizeof(inode));
            write_block(global_diskptr, inode_block_idx, (void*)c);
            free(c);
            free(A);
            break;
        }
        free(A);
    }
    free(sb);
    if(!flag){
        // No empty bitmap found :(
            return -1;
    }
    return 0;
}

int stat(int inumber) {
    if(global_diskptr == NULL) {
		return -1;
	}

    // Read Super Block
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, 0, (void *)c);
    super_block* sb = (super_block*)malloc(sizeof(super_block));
    memcpy(sb, c, sizeof(super_block));

    int inode_block_idx = sb->inode_block_idx + inumber/128;
    int inode_i = inumber%128;

    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, inode_block_idx, (void *)c);
    inode* __inode = (inode*)malloc(sizeof(inode));
    memcpy(__inode, c+(32*inode_i), sizeof(inode));

    int logical_size = __inode->size;
    int num_direct_pointer = min(ceil(logical_size,(BLOCKSIZE)), 5);
    int num_indirect_pointer = (logical_size <= 5*BLOCKSIZE) ? 0 : 1;
    int data_blocks = ceil(logical_size,(BLOCKSIZE));

    printf("Logical Size: %d\n", __inode->size);
    printf("Number of Direct Pointers: %d\n", num_direct_pointer);
    printf("Number of Indirect Pointers: %d\n", num_indirect_pointer);
    printf("Number of Data Blocks: %d\n", data_blocks);

    free(__inode);
    return 0;
}

int read_i(int inumber, char *data, int length, int offset);

int write_i(int inumber, char *data, int length, int offset);

int remove_file(int inumber);