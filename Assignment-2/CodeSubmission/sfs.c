#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#include "sfs.h"

#define ceil(a,b) ((a+b-1)/b)

int min(int a, int b) {
    if(a <= b) {
        return a;
    }
    return b;
}

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
            int inode_idx = BLOCKSIZE*8*(bitmap_block_i-1) + 32*bits_i + k;
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

    c = (char *)malloc(BLOCKSIZE*sizeof(char));
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

int is_valid(super_block* sb, int inumber, int offset) {
    if(inumber >= sb->inodes || inumber < 0) {
        return 0;
    }

    if(offset >= 1029*BLOCKSIZE || offset < 0) {
        return 0;
    }
    return 1;
}

int find_empty_data_block(super_block* sb, inode* __inode) {
    int data_bitmap_block_i=sb->data_block_bitmap_idx, bits_i=0, flag=0;

    for(; data_bitmap_block_i < sb->inode_block_idx ; data_bitmap_block_i++){

        int* A = (int *)calloc(BLOCKSIZE, sizeof(char));
        if(read_block(global_diskptr, data_bitmap_block_i, (void *)A)!=0){
            return -1;
        }
        int k=0;
        for(bits_i = 0; bits_i<BLOCKSIZE/4;bits_i++){
            for(k=0;k<32;k++){
                if(!( A[(bits_i)] & (1 << k) )){
                    // Unset Bit found
                    A[bits_i] |= (1 << k);
                    flag=1;
                    // Update the data bitmap
                    if(write_block(global_diskptr, data_bitmap_block_i, (void *)A)!=0){
                        free(A);
                        return -1;
                    }
                    break;
                }
            }
            if(flag)
                break;
        }
        if(flag) {
            int data_idx = BLOCKSIZE*8*(data_bitmap_block_i - sb->data_block_bitmap_idx) + 32*bits_i + k;
            free(A);
            return data_idx;
        }
    }
    
    if(!flag){
        // No empty bitmap found :(
            return -1;
    }

    return 0;
}


int write_i(int inumber, char *data, int length, int offset) {
    //TODO: Set valid bit  1
    printf("\n IN WRITE_I \n");
    if(global_diskptr == NULL) {
		return -1;
	}


    // Read Super Block
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, 0, (void *)c);
    super_block* sb = (super_block*)malloc(sizeof(super_block));
    memcpy(sb, c, sizeof(super_block));
    print_superblock_info(sb);
    printf("\n");

    // Checking if inumber and offset are valid
    if(!is_valid(sb, inumber, offset)) {
        return -1;
    }

    // Finding the respective inode
    int inode_block_idx = sb->inode_block_idx + inumber/128;
    int inode_i = inumber%128;
    // c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, inode_block_idx, (void *)c);
    inode* __inode = (inode*)malloc(sizeof(inode));
    memcpy(__inode, c+(32*inode_i), sizeof(inode));

    // stats
    int logical_size = __inode->size;
    int num_direct_pointer = min(ceil(logical_size,(BLOCKSIZE)), 5);
    int num_indirect_pointer = (logical_size <= 5*BLOCKSIZE) ? 0 : 1;
    int data_blocks = ceil(logical_size,(BLOCKSIZE)); //+ num_indirect_pointer;

    char* traverse = data;
    int total = length;
    int b_index = 0;
    int block_index;

    int allocate_indirect = 0;
    int* indirect_pointer;
    int block_index_indirect;

    while(length > 0) {
        printf("Length left in bytes: %d\n", length);
        // Allocating block memory if not there
        if(b_index >= data_blocks) {
            if(b_index < 5) {
                // Allo cate direct pointer
                // int index = function_to_find_available_data_bitmap_idx()
                block_index = find_empty_data_block(sb, __inode);
                printf("BLOCK INDEXAA %d\n", block_index);
                if(block_index < 0) {
                    printf("Memory Not available\n");
                    return total - length;
                }
                data_blocks ++;
                printf("Empty data block index: %d\n", block_index);
                __inode->direct[b_index] = block_index;
            }
            else {
                // Allocate indirect pointer
                // We need to find two empty data blocks 
                // function_to_find_available_data_bitmap_idx()                

                if(!allocate_indirect) {
                    // 1. Finding empty data block for indirect pointer
                    block_index_indirect = find_empty_data_block(sb, __inode);
                    if(block_index < 0) {
                        printf("Memory Not available\n");
                        return total - length;
                    }

                    indirect_pointer = (int *)malloc(1024*sizeof(int));

                    write_block(global_diskptr, sb->data_block_idx + block_index_indirect, (void *)indirect_pointer);
                    __inode->indirect = block_index_indirect;

                    allocate_indirect = 1;
                }
                else {
                    indirect_pointer = (int *)malloc(1024*sizeof(int));
                    read_block(global_diskptr, sb->data_block_idx + block_index_indirect, (void *)indirect_pointer);
                }

                // Finding empty block for pointer in indirect
                block_index = find_empty_data_block(sb, __inode);
                if(block_index < 0) {
                    // TODO: if b_index == 5 and data block is not available 
                    printf("Memory Not available\n");
                    return total - length;
                }
                data_blocks++;
    
                
                indirect_pointer[b_index%5] = block_index;
                write_block(global_diskptr, sb->data_block_idx + block_index_indirect, (void *)indirect_pointer);
            }
        }
        //Find memory block
        else {
            // find the block
            if(b_index < 5) {
                // Allo cate direct pointer
                // int index = function_to_find_available_data_bitmap_idx()
                block_index = __inode->direct[b_index];
            }
            else {
                int* temp = (int *)malloc((BLOCKSIZE/4)*sizeof(int));
                read_block(global_diskptr, sb->data_block_idx + __inode->indirect, (void *)temp);
                block_index = temp[b_index%5];
            }
        }
///////////////////////////////////////////////////////////////////////////////////////////////
        printf("BLOCK INDEX %d\n", block_index);
        // we'll have the pointer to block currently allocated

        if(offset >= b_index*BLOCKSIZE && offset < (b_index+1)*BLOCKSIZE) {
            // Write to block number we found or allocated above 
            // write_to_block_i(data, min(length, (b_index+1)*Blocksize - offset))
            // data += min(length, (b_index+1)*Blocksize - offset)
            // length -= min(length, (b_index+1)*Blocksize - offset)
            // offset == (b_index+1)*Blocksize
            if((b_index+1)*BLOCKSIZE - offset < BLOCKSIZE) {
                // Partial Write
                printf("PARTIAL WRITE\n");

                int block_offset = (b_index+1)*BLOCKSIZE - offset;

                char* temp = (char *)malloc(BLOCKSIZE*sizeof(char));
                read_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);

                memcpy(temp+block_offset, traverse, min(length, block_offset));

                write_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);

                length -= min(length, block_offset);
                traverse += min(length, block_offset);
                
            }
            else {
                printf("FULL WRITE\n");
                printf("BLOCK INDEX %d\n", block_index);
                printf("BLOCK WRITE NUMBER %D\n", sb->data_block_idx + block_index);

                // Reading data already existing in block
                char* temp = (char *)malloc(BLOCKSIZE*sizeof(char));
                read_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);

                // Overwriting the required part
                memcpy(temp, traverse, min(length, BLOCKSIZE));
                
                write_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);

                length -= min(length, BLOCKSIZE);
                traverse += min(length, BLOCKSIZE);
            }
            offset = (b_index + 1)*BLOCKSIZE;
        }
        
        b_index++;
    }
    __inode->size = data_blocks*BLOCKSIZE;
    memcpy(c+(32*inode_i), __inode, sizeof(inode));
    write_block(global_diskptr, inode_block_idx, (void *)c);

    return total - length;
}

int read_i(int inumber, char *data, int length, int offset) {
    printf("\n IN READ_I \n");
    if(global_diskptr == NULL) {
		return -1;
	}

    // Finding super blocl
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, 0, (void *)c);
    super_block* sb = (super_block*)malloc(sizeof(super_block));
    memcpy(sb, c, sizeof(super_block));

    //TODO: is_valid needs to be chaned for read_i 
    // Checking if inumber and offset are valid
    if(!is_valid(sb, inumber, offset)) {
        return -1;
    }

    //Finding the required __innode
    int inode_block_idx = sb->inode_block_idx + inumber/128;
    int inode_i = inumber%128;
    c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, inode_block_idx, (void *)c);
    inode* __inode = (inode*)malloc(sizeof(inode));
    memcpy(__inode, c+(32*inode_i), sizeof(inode));

    int logical_size = __inode->size;
    int data_blocks = ceil(logical_size,(BLOCKSIZE)); 

    // char* a = (char *)malloc(BLOCKSIZE*sizeof(char));
    // read_block(global_diskptr, sb->data_block_idx + __inode->direct[0], (void *)a);

    char* traverse = data;
    int total = length;
    int b_index = 0;
    int block_index;

    while(length > 0) {
        printf("Length left in bytes: %d\n", length);
        // we'll have the pointer to block currently allocated

        if(b_index >= data_blocks) {
            printf("Complete File Read\n");
            printf("DATA\n");
            printf("%c\n", data[0]);
            printf("TOTAL %d\n", total);
            printf("LENGTH %d\n", length);
            for(int i=0;i<(total - length);++i) {
                printf("%c\n", data[i]);
            }
            return 0;
        }

        if(b_index < 5) {
            block_index = __inode->direct[b_index];        
        }
        else {
            int* temp = (int *)malloc((BLOCKSIZE/4)*sizeof(int));
            printf("INDIRECT BLOCK ID %d\n", sb->data_block_idx + __inode->indirect);
            read_block(global_diskptr, sb->data_block_idx + __inode->indirect, (void *)temp);
            printf("TEMPPPPP %d\n", temp[0]);
            block_index = temp[b_index%5];       
        }
        printf("BLOCK INDEX READ %d\n", block_index);
        if(offset >= b_index*BLOCKSIZE && offset < (b_index+1)*BLOCKSIZE) {
            // Write to block number we found or allocated above 
            // write_to_block_i(data, min(length, (b_index+1)*Blocksize - offset))
            // data += min(length, (b_index+1)*Blocksize - offset)
            // length -= min(length, (b_index+1)*Blocksize - offset)
            // offset == (b_index+1)*Blocksize

            if((b_index+1)*BLOCKSIZE - offset < BLOCKSIZE) {
                // Partial Write
                printf("PARTIAL READ\n");

                int block_offset = (b_index+1)*BLOCKSIZE - offset;

                char* temp = (char *)malloc(BLOCKSIZE*sizeof(char));
                read_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);

                memcpy(traverse, temp+block_offset, min(length, block_offset));

                length -= min(length, block_offset);
                traverse += min(length, block_offset);
                
            }
            else {
                printf("FULL READ\n");
                printf("BLOCK WRITE NUMBER %d\n", sb->data_block_idx + block_index);

                // Reading data already existing in block
                char* temp = (char *)malloc(BLOCKSIZE*sizeof(char));
                read_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);

                // Overwriting the required part
                printf("MINN %d\n", min(length, BLOCKSIZE));
                memcpy(traverse, temp, min(length, BLOCKSIZE));
                
                length -= min(length, BLOCKSIZE);
                traverse += min(length, BLOCKSIZE);
            }
            offset = (b_index + 1)*BLOCKSIZE;
        }

        b_index++;
    }

    printf("Complete length Read\n");
    printf("DATA\n");
    printf("TOTAL %d\n", total);
    printf("LENGTH %d\n", length);
    int count = 0;
    for(int i=0;i<(total - length);++i) {
        if(data[i] == 'a') {
            count ++;
        }
        // printf("character %c\n", data[i]);
    }
    printf("COUNT: %d\n", count);
    return 0;
}

// Util function for handling the data bit map;
void unset_kth_bit(super_block* sb, int block_index, int offset){

    // 8*BLOCKSIZE - number of bits in one block

    int bitmap_block_i = offset + block_index / 8*BLOCKSIZE;
    int data_bit_i = block_index % (8*BLOCKSIZE);

    int* A = (int *)calloc(BLOCKSIZE, sizeof(char));
    if(read_block(global_diskptr, bitmap_block_i, (void *)A)!=0){
        exit(-1);
    }

    int int_index = data_bit_i / 32; // one int is 32 bits and thats how bit map is implemented
    int int_index_bit = data_bit_i % 32; // index of the bit in the int

    A[int_index] ^=  (1 << int_index_bit);

    write_block(global_diskptr, bitmap_block_i, (void *)A);
}


void print_bitmap_info(super_block* sb, int inumber) {
    int inode_block_idx = sb->inode_block_idx + inumber/128;
    int inode_i = inumber%128;
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, inode_block_idx, (void *)c);
    inode* __inode = (inode*)malloc(sizeof(inode));
    memcpy(__inode, c+(32*inode_i), sizeof(inode));

    printf("VALID BIT %d\n", __inode->valid);

    int bitmap_block_i = 1 + inumber / 8*BLOCKSIZE;

    int* A = (int *)calloc(BLOCKSIZE, sizeof(char));
    if(read_block(global_diskptr, bitmap_block_i, (void *)A)!=0){
        exit(-1);
    }

    int bits_i;
    int k;
    int count = 0;
    for(bits_i = 0; bits_i<BLOCKSIZE/4;bits_i++){
        for(k=0;k<32;k++){
            if(( A[(bits_i)] & (1 << k) )){
                count ++;
            }
        }
    }

    printf("Number of bits set to one in inode bitsmap %d\n", count);


    bitmap_block_i = sb->data_block_bitmap_idx;

    A = (int *)calloc(BLOCKSIZE, sizeof(char));
    if(read_block(global_diskptr, bitmap_block_i, (void *)A)!=0){
        exit(-1);
    }

    count = 0;
    for(bits_i = 0; bits_i<BLOCKSIZE/4;bits_i++){
        for(k=0;k<32;k++){
            if(( A[(bits_i)] & (1 << k) )){
                count ++;
            }
        }
    }

    printf("Number of bits set to one in data bitsmap %d\n", count);

}

int remove_file(int inumber) {
    
    printf("\nIN remove_file \n");
    if(global_diskptr == NULL) {
		return -1;
	}

    // Finding super block
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, 0, (void *)c);
    super_block* sb = (super_block*)malloc(sizeof(super_block));
    memcpy(sb, c, sizeof(super_block));
    print_bitmap_info(sb, inumber);
    // Checking if inumber and offset are valid
    // if(!is_valid(global_diskptr, sb, inumber)) {
    //     return -1;
    // }

    //Finding the required __innode
    int inode_block_idx = sb->inode_block_idx + inumber/128;
    int inode_i = inumber%128;
    c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, inode_block_idx, (void *)c);
    inode* __inode = (inode*)malloc(sizeof(inode));
    memcpy(__inode, c+(32*inode_i), sizeof(inode));

    // Valid Bit -> 0
    __inode->valid = 0;

    // Update the indode
    memcpy(c+(32*inode_i), __inode, sizeof(inode));
    write_block(global_diskptr, inode_block_idx, (void *)c);

    int data_blocks = ceil(__inode->size, BLOCKSIZE);    
    int b_index = 0;
    int indirect_block_id;
    int block_index;

    int* temp;
    // get the Inode indirect pointer data
    if(data_blocks > 5) {
        indirect_block_id = __inode->indirect;
        temp = (int *)malloc((BLOCKSIZE/4)*sizeof(int));
        read_block(global_diskptr, sb->data_block_idx + __inode->indirect, (void *)temp);
    }

    while(b_index < data_blocks) {
        if(b_index < 5) {
            block_index = __inode->direct[b_index];
        }
        else {
            block_index = temp[b_index%5];
        }
        unset_kth_bit(sb, block_index, sb->data_block_bitmap_idx);
        b_index++;
    }

    unset_kth_bit(sb, indirect_block_id, sb->data_block_bitmap_idx); // Unsetting the bit for indirect pointer

    unset_kth_bit(sb, inumber, 1); //Unsetting the bit in inode bit map

    print_bitmap_info(sb, inumber);
    return 0;

}