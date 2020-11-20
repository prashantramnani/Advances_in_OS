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

int max(int a, int b) {
    if(a < b) {
        return b;
    }
    return a;
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
        inodes[i].size = 0;
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
    // Setting values for root directory
    inodes[0].valid = 1;
    // inodes[0].direct[0] = 0;
    // inodes[0].size = sizeof(dir_entry)*entries_per_block;
    inodes[0].size = 0;
    // write inode to disk
    for(int i=0;i<sb->inode_blocks;++i) {
        if(write_block(diskptr, sb->inode_block_idx + i, (void *)inodes)!=0)
            return -1;    
    }
    
    // initialise bitmap
    char* C2 = (char *)calloc(BLOCKSIZE, sizeof(char));

    // initialise datablock bitmap
    for(int i=sb->data_block_bitmap_idx;i<sb->inode_block_idx;i++){
        if(write_block(diskptr, i, (void *)C2)!=0)
            return -1;  
    }

    C2[0] = C2[0] || (1);
    // initialise inode bitmap
    for(int i=1;i<sb->data_block_bitmap_idx;i++){
        if(write_block(diskptr, i, (void *)C2)!=0)
            return -1;
    }
    
    free(C2);
    
    // Initialising the root data block
    // dir_entry de[entries_per_block];
    // for(int i=0;i<entries_per_block;++i) {
    //     de[i].meta_data = 0;
    // }

    // if(write_block(diskptr, sb->data_block_idx, (void *)de)!=0) {
    //     return -1;
    // }

    return 0;
}

super_block* find_super_block() {
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, 0, (void *)c);
    super_block* sb = (super_block*)malloc(sizeof(super_block));
    memcpy(sb, c, sizeof(super_block));

    return sb;
}

inode* find_inode(super_block* sb, int inumber) {
    // Finding the respective inode
    int inode_block_idx = sb->inode_block_idx + inumber/128;
    int inode_i = inumber%128;
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, inode_block_idx, (void *)c);
    inode* __inode = (inode*)malloc(sizeof(inode));
    memcpy(__inode, c+(32*inode_i), sizeof(inode));

    return __inode;
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

    super_block* sb = find_super_block();

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
            //printf("KK %d\n", k);
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
            free(sb);
            return inode_idx;
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
    super_block* sb = find_super_block();
    if(inumber >= sb->inodes) {
        printf("Invalid Inode\n");
        return -1;
    }

    inode* __inode = find_inode(sb, inumber);
    // Check if inode is valid
    if(__inode->valid == 0) {
        printf("Invalid Inode\n");
        return -1;
    }

    int logical_size = __inode->size;
    int num_direct_pointer = min(ceil(logical_size,(BLOCKSIZE)), 5);
    int num_indirect_pointer = (logical_size <= 5*BLOCKSIZE) ? 0 : 1;
    int data_blocks = ceil(logical_size,(BLOCKSIZE));

    printf("Logical Size: %d\n", __inode->size);
    printf("Valid Bit: %d\n", __inode->valid);
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

int find_empty_data_block(super_block* sb) {
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
    if(global_diskptr == NULL) {
		return -1;
	}
    dir_entry* de = (dir_entry *)malloc(sizeof(dir_entry));
    memcpy(de, data, sizeof(dir_entry));

    super_block* sb = find_super_block();

    // Checking if inumber and offset are valid
    if(!is_valid(sb, inumber, offset)) {
        return -1;
    }

    int inode_block_idx = sb->inode_block_idx + inumber/128;
    int inode_i = inumber%128;
    
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, inode_block_idx, (void *)c);
    inode* __inode = (inode*)malloc(sizeof(inode));
    memcpy(__inode, c+(32*inode_i), sizeof(inode));
    __inode->valid = 1;

    // stats
    int logical_size = __inode->size;
    int num_direct_pointer = min(ceil(logical_size,(BLOCKSIZE)), 5);
    int num_indirect_pointer = (logical_size <= 5*BLOCKSIZE) ? 0 : 1;
    int data_blocks = ceil(logical_size,(BLOCKSIZE)); 

    char* traverse = data;
    int total = length;
    int b_index = 0;
    int block_index;

    int allocate_indirect = 0;
    int* indirect_pointer;
    int block_index_indirect;

    while(length > 0) {
        // Allocating block memory if not there
        if(b_index >= data_blocks) {
            if(b_index < 5) {
                // Allocate direct pointer
                // int index = function_to_find_available_data_bitmap_idx()
                block_index = find_empty_data_block(sb);
                if(block_index < 0) {
                    printf("Memory Not Available\n");
                    return total - length;
                }
                data_blocks ++;
                __inode->direct[b_index] = block_index;
            }
            else {
                // Allocate indirect pointer
                // We need to find two empty data blocks 
                // function_to_find_available_data_bitmap_idx()                

                if(!allocate_indirect) {
                    // 1. Finding empty data block for indirect pointer
                    block_index_indirect = find_empty_data_block(sb);
                    if(block_index < 0) {
                        printf("Memory Not Available\n");
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
                block_index = find_empty_data_block(sb);
                if(block_index < 0) {
                    unset_kth_bit(sb, sb->data_block_bitmap_idx + block_index_indirect/(8*BLOCKSIZE), block_index_indirect%(8*BLOCKSIZE));
                    printf("Memory Not Available\n");
                    return total - length;
                }
                data_blocks++;
                
                indirect_pointer[b_index - 5] = block_index;
                write_block(global_diskptr, sb->data_block_idx + block_index_indirect, (void *)indirect_pointer);
            }
        }
        //Find memory block
        else {
            // find the block
            if(b_index < 5) {
                // Allocate direct pointer
                // int index = function_to_find_available_data_bitmap_idx()
                block_index = __inode->direct[b_index];
            }
            else {
                int* temp = (int *)malloc((BLOCKSIZE/4)*sizeof(int));
                read_block(global_diskptr, sb->data_block_idx + __inode->indirect, (void *)temp);
                block_index = temp[b_index - 5];
            }
        }
///////////////////////////////////////////////////////////////////////////////////////////////
        // we'll have the pointer to block currently allocated
        if(offset >= b_index*BLOCKSIZE && offset < (b_index+1)*BLOCKSIZE) {
            // Write to block number we found or allocated above 
            // write_to_block_i(data, min(length, (b_index+1)*Blocksize - offset))
            // data += min(length, (b_index+1)*Blocksize - offset)
            // length -= min(length, (b_index+1)*Blocksize - offset)
            // offset == (b_index+1)*Blocksize
            if((b_index+1)*BLOCKSIZE - offset < BLOCKSIZE) {
                // Partial Write
                int block_offset = (b_index+1)*BLOCKSIZE - offset;

                char* temp = (char *)malloc(BLOCKSIZE*sizeof(char));
                read_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);

                memcpy(temp+offset - b_index*BLOCKSIZE, traverse, min(length, block_offset));

                write_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);
            
            }
            else {
                // FULL WRITE

                // Reading data already existing in block
                char* temp = (char *)malloc(BLOCKSIZE*sizeof(char));
                read_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);

                // Overwriting the required part
                memcpy(temp, traverse, min(length, BLOCKSIZE));
                
                write_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);
            }
            int x = length;
            length -= min(x, BLOCKSIZE);
            traverse += min(x, BLOCKSIZE);
            offset += min(x, BLOCKSIZE);
        }
        
        b_index++;
    }
    //data_blocks
    // max(offset + length, data_blocks*BLOCKSIZE)
    int size = max(__inode->size, min(offset, data_blocks*BLOCKSIZE));
    __inode->size = size;
    memcpy(c+(32*inode_i), __inode, sizeof(inode));
    write_block(global_diskptr, inode_block_idx, (void *)c);

    return total - length;
}

int read_i(int inumber, char *data, int length, int offset) {
    // printf("\n IN READ_I \n");
    if(global_diskptr == NULL) {
		return -1;
	}

    // Finding super blocl
    super_block* sb = find_super_block();

    // Checking if inumber and offset are valid
    if(!is_valid(sb, inumber, offset)) {
        return -1;
    }

    //Finding the required __innode
    inode* __inode = find_inode(sb, inumber);
    if(__inode->valid == 0) {
        printf("Invalid Inode\n");
        return -1;
    }

    int logical_size = __inode->size;
    int data_blocks = ceil(logical_size,(BLOCKSIZE)); 

    char* traverse = data;
    int total = length;
    int b_index = 0;
    int block_index;

    while(length > 0) {
        // we'll have the pointer to block currently allocated
        if(b_index < 5) {
            block_index = __inode->direct[b_index];        
        }
        else {
            int* temp = (int *)malloc((BLOCKSIZE/4)*sizeof(int));
            read_block(global_diskptr, sb->data_block_idx + __inode->indirect, (void *)temp);
            block_index = temp[b_index -5];       
        }

        if(offset >= b_index*BLOCKSIZE && offset < (b_index+1)*BLOCKSIZE) {
            // Write to block number we found or allocated above 
            // write_to_block_i(data, min(length, (b_index+1)*Blocksize - offset))
            // data += min(length, (b_index+1)*Blocksize - offset)
            // length -= min(length, (b_index+1)*Blocksize - offset)
            // offset == (b_index+1)*Blocksize

            if((b_index+1)*BLOCKSIZE - offset < BLOCKSIZE) {
                // Partial Write
                int block_offset = (b_index+1)*BLOCKSIZE - offset;

                char* temp = (char *)malloc(BLOCKSIZE*sizeof(char));
                read_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);

                memcpy(traverse, temp + offset - b_index*BLOCKSIZE, min(length, block_offset));

                length -= min(length, block_offset);
                traverse += min(length, block_offset);
                
            }
            else {
                // FULL READ
                // Reading data already existing in block
                char* temp = (char *)malloc(BLOCKSIZE*sizeof(char));
                read_block(global_diskptr, sb->data_block_idx + block_index, (void *)temp);

                // Overwriting the required part
                memcpy(traverse, temp, min(length, BLOCKSIZE));
                
                length -= min(length, BLOCKSIZE);
                traverse += min(length, BLOCKSIZE);
            }
            offset = (b_index + 1)*BLOCKSIZE;
        }

        b_index++;
    }

    return total - length;
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

    A[(data_bit_i/32)] &= ~(1 << (data_bit_i%32));
    write_block(global_diskptr, bitmap_block_i, (void *)A);
}


void print_bitmap_info(super_block* sb, int inumber) {
    int inode_block_idx = sb->inode_block_idx + inumber/128;
    int inode_i = inumber%128;
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, inode_block_idx, (void *)c);
    inode* __inode = (inode*)malloc(sizeof(inode));
    memcpy(__inode, c+(32*inode_i), sizeof(inode));

    // printf("VALID BIT %d\n", __inode->valid);

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

    //printf("Number of bits set to one in inode bitsmap %d\n", count);


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

    //printf("Number of bits set to one in data bitsmap %d\n", count);

}

int remove_file(int inumber) {
    if(global_diskptr == NULL) {
		return -1;
	}

    // Finding super block
    super_block* sb = find_super_block();
    
    //Finding the required __innode
    int inode_block_idx = sb->inode_block_idx + inumber/128;
    int inode_i = inumber%128;
    char* c = (char *)malloc(BLOCKSIZE*sizeof(char));
    read_block(global_diskptr, inode_block_idx, (void *)c);
    inode* __inode = (inode*)malloc(sizeof(inode));
    memcpy(__inode, c+(32*inode_i), sizeof(inode));

    if(inumber >= sb->inodes || inumber < 0 || __inode->valid == 0) {
        return -1;
    }

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
            block_index = temp[b_index - 5];
        }
        unset_kth_bit(sb, block_index, sb->data_block_bitmap_idx);
        b_index++;
    }

    if(data_blocks > 5) {
        unset_kth_bit(sb, indirect_block_id, sb->data_block_bitmap_idx); // Unsetting the bit for indirect pointer
    }

    unset_kth_bit(sb, inumber, 1); //Unsetting the bit in inode bit map

    return 0;

}

int parse_path(char* dirpath) {
    if(dirpath[0] != '/') {
        return -1;
    }

    int slash = 0;
    for(int i=0;dirpath[i] != EOF; i++) {
        if(dirpath[i] == '/') {
            slash ++;
        }
    }
    return slash;
}

int find_file(super_block* sb, int curr_inode, char* filename, int type) {

    inode* __inode = find_inode(sb, curr_inode);

    int data_blocks = ceil(__inode->size, BLOCKSIZE);
    int b_index = 0;

    dir_entry *de = (dir_entry *)malloc(sizeof(dir_entry));
    int remaining_entries = __inode->size/32;
    int pr = 0;
    while(b_index < data_blocks) {
        if(b_index < 5) {
            int d_block = __inode->direct[b_index];
            char* temp = (char *)malloc(BLOCKSIZE);
            read_block(global_diskptr, sb->data_block_idx + d_block, (void *)temp);
            for(int i=0;i<min(entries_per_block, remaining_entries) ;++i) {
                memcpy(de, temp + i*32, 32);
                int t = de->meta_data >> 1;
                int x=1;
                if(t%2 == 0) {
                    x = 0;
                }
                if(!(x ^ type )) {
                    if(!strcmp(de->filename, filename)) {
                        return de->inode;
                    }
                }
            }
            remaining_entries -= min(entries_per_block, remaining_entries);
        }
        else {
            int* x = (int *)malloc((BLOCKSIZE/4)*sizeof(int));
            read_block(global_diskptr, sb->data_block_idx + __inode->indirect, (void *)x);
            int d_block = x[b_index - 5];  

            char* temp = (char *)malloc(BLOCKSIZE);
            read_block(global_diskptr, sb->data_block_idx + d_block, (void *)temp);

            for(int i=0;i<min(entries_per_block, remaining_entries) ;++i) {
                memcpy(de, temp + i*32, 32);;
                int t = de->meta_data >> 1;
                int x=1;
                if(t%2 == 0) {
                    x = 0;
                }
                if(!(x ^ type )) {
                    if(!strcmp(de->filename, filename)) {
                        return de->inode;
                    }
                }
            }
            remaining_entries -= min(entries_per_block, remaining_entries);
        }
        b_index ++ ;
    }

    //printf("NO FILE FOUNDm INVALID ADDRESS\n");
    return -1;
}

void insert_dir_entry(super_block* sb, int prev_inode, int new_dir_inode, char* filename, int type) {
    inode* __inode = find_inode(sb, prev_inode);

    // int data_blocks = ceil(__inode->size, BLOCKSIZE);

    // creating an entry and initialising it
    dir_entry *de = (dir_entry *)calloc(1, (sizeof(dir_entry)));
    de->meta_data |= 1;
    de->meta_data |= (type << 1);
    de->meta_data |= (strlen(filename) << 2);
    de->inode = new_dir_inode; 
    strcpy(de->filename, filename);


    // TODO check for invalid bit 
    int offset = __inode->size;
    // write_i(prev_inode, (char *)de, sizeof(dir_entry), offset);


    // Checking for first invalid entry
    int data_blocks = ceil(__inode->size, BLOCKSIZE);
    int b_index = 0;

    dir_entry *temp_de = (dir_entry *)malloc(__inode->size);
    int remaining_entries = __inode->size/32;

    while(b_index < data_blocks) {
        if(b_index < 5) {
            int d_block = __inode->direct[b_index];
            char* temp = (char *)malloc(BLOCKSIZE);
            read_block(global_diskptr, sb->data_block_idx + d_block, (void *)temp);

            for(int i=0;i<min(entries_per_block, remaining_entries) ;++i) {
                memcpy(temp_de, temp + i*32, 32);;
                if(!(temp_de->meta_data & 1)) {
                    offset = min(offset, b_index*BLOCKSIZE + i*32);
                }
            }
            remaining_entries -= min(entries_per_block, remaining_entries);
        }
        else {
            int* x = (int *)malloc((BLOCKSIZE/4)*sizeof(int));
            read_block(global_diskptr, sb->data_block_idx + __inode->indirect, (void *)x);
            int d_block = x[b_index - 5];  

            char* temp = (char *)malloc(BLOCKSIZE);
            read_block(global_diskptr, sb->data_block_idx + d_block, (void *)temp);

            for(int i=0;i<min(entries_per_block, remaining_entries) ;++i) {
                memcpy(temp_de, temp + i*32, 32);;
                if(!(temp_de->meta_data & 1)) {
                    offset = min(offset, b_index*BLOCKSIZE + i*32);
                }
            }
            remaining_entries -= min(entries_per_block, remaining_entries);
        }
        b_index ++ ;
    }
    //printf("OFFSER WHILE WRITING %d\n", offset);
    write_i(prev_inode, (char *)de, sizeof(dir_entry), offset);

}

void print_rootdir_data(super_block* sb, int inumber) {
    
    inode* __inode = find_inode(sb, inumber);
    char* temp = (char *)malloc(__inode->size);
    printf("INODE: %d\n", inumber);
    printf("INODE SIZE: %d\n", __inode->size);
    read_i(inumber, temp, __inode->size, 0);
    dir_entry de;
    for(int i=0;i<__inode->size/32;++i) {
        memcpy(&de, temp + i*32, 32);
        printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
        printf("FILE NAME INODE IN ROOT %d\n", de.inode);
        printf("FILE NAME IN ROOT %s\n", de.filename);
        if(__inode->size > 0) {
            print_rootdir_data(sb, de.inode);
        }
    }

}

int create_dir(char *dir) {
    super_block* sb = find_super_block();

    int curr_inode = 0, prev_inode = curr_inode;
    int string_i = 0;
    char filename[27];

    while(dir[string_i]!='\0'){        

        while(dir[string_i]=='/'){
            string_i++;
        }    
        int start = string_i, final=start;
        while(dir[string_i]!='/' && dir[string_i]!='\0'){
            string_i++;
        }
        final=string_i-1;

        memset(filename, '\0', 27);

        int i;
        for(i=0;i<26 && start+i<=final;i++){
            filename[i] = dir[start+i];
        }
        filename[i]='\0';
        // printf("%s \t %d\n", filename, curr_inode);

        prev_inode = curr_inode;
        curr_inode = find_file(sb, curr_inode, filename, 1);
        if(curr_inode<0){
            break;
        }
    }

    if(curr_inode<0){
        while(dir[string_i] == '/') {
            string_i++;
        }
        if(dir[string_i]!='\0'){
            // intermediate dir DNE 
            // ERROR
            return -1;
        }
        else{
            // CREATE FILE at curr inode with filename
            // printf("Filename - %s\n", filename);

            int new_dir_inode = create_file();
            // printf("NEW DIR INDOE %d\n", new_dir_inode);
            insert_dir_entry(sb, prev_inode, new_dir_inode, filename, 1);
        }
    }


    // print_rootdir_data(sb, 0);
    // printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    return 0;
}

int write_file(char *dir, char *data, int length, int offset) {
    // Parse Filepath
    // If file does not exist then create a file and return inode or else just return inode
    // do write_i for file with given legth and offset
    

    super_block* sb = find_super_block();

    int curr_inode = 0, prev_inode = curr_inode;
    int string_i = 0;
    char filename[27];

    while(dir[string_i]!='\0'){        

        while(dir[string_i]=='/'){
            string_i++;
        }    
        int start = string_i, final=start;
        while(dir[string_i]!='/' && dir[string_i]!='\0'){
            string_i++;
        }
        final=string_i-1;

        memset(filename, '\0', 27);

        int i;
        for(i=0;i<26 && start+i<=final;i++){
            filename[i] = dir[start+i];
        }
        filename[i]='\0';
        //printf("%s \t %d\n", filename, curr_inode);

        prev_inode = curr_inode;
        curr_inode = find_file(sb, curr_inode, filename, 1);
        if(curr_inode<0){
            break;
        }
    }
    curr_inode = find_file(sb, prev_inode, filename, 0);
    if(curr_inode<0){
        while(dir[string_i] == '/') {
            string_i++;
        }
        if(dir[string_i]!='\0'){
            // intermediate dir DNE 
            // ERROR
            return -1;
        }
        else{
            // CREATE FILE at curr inode with filename
            //printf("Filename - %s\n", filename);

            int new_file_inode = create_file();
            //printf("NEW DIR INDOE %d\n", new_dir_inode);
            insert_dir_entry(sb, prev_inode, new_file_inode, filename, 0);

            return write_i(new_file_inode, data, length, offset);
        }
    }
    else {
        return write_i(curr_inode, data, length, offset);
    }
    // print_rootdir_data(sb, 0);
    //printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
    return 0;
}

int read_file(char *dir, char *data, int length, int offset) {
    // parse filepath and check if file exists
    // return -1 if does not exist
    // find inode of file
    // read_i 
    
    super_block* sb = find_super_block();

    int curr_inode = 0, prev_inode = curr_inode;
    int string_i = 0;
    char filename[27];

    while(dir[string_i]!='\0'){        

        while(dir[string_i]=='/'){
            string_i++;
        }    
        int start = string_i, final=start;
        while(dir[string_i]!='/' && dir[string_i]!='\0'){
            string_i++;
        }
        final=string_i-1;

        memset(filename, '\0', 27);

        int i;
        for(i=0;i<26 && start+i<=final;i++){
            filename[i] = dir[start+i];
        }
        filename[i]='\0';
        //printf("%s \t %d\n", filename, curr_inode);

        prev_inode = curr_inode;
        curr_inode = find_file(sb, curr_inode, filename, 1);
        if(curr_inode<0){
            break;
        }
    }
    curr_inode = find_file(sb, prev_inode, filename, 0);
    
    if(curr_inode<0){
        return -1;
        //printf("FILE NOT FOUND\n");
    }
    else {
        return read_i(curr_inode, data, length, offset);
    }

    return 0;
}

void delete_recursive(super_block* sb, int inumber) {
    dir_entry* de = (dir_entry *)malloc(sizeof(dir_entry));
    inode* __inode = find_inode(sb, inumber);
    char* temp = (char *)malloc(__inode->size);
    //printf("INODE SIZE: %d\n", __inode->size);
    
    read_i(inumber, temp, __inode->size, 0);
    // dir_entry d;
    // memcpy(&d, temp + i*32, 32);
    for(int i=0;i < __inode->size/32;++i) {
        memcpy(de, temp + i*32, 32);

        int t = de->meta_data >> 1;
        int x=1;
        if(t%2 == 0) {
            x = 0;
        }
        if(x == 0) {
            int err = remove_file(de->inode);        
            continue; // If the current file is not a directory, don't go into recursion
        }
        if(__inode->size > 0) {
            delete_recursive(sb, de->inode);
        }
    }
    //printf("REMOVING FILENAME WITH INODE: %d\n", inumber);
    int err = remove_file(inumber);

    free(de);
    free(temp);
    free(__inode);
    // printf("FILE NAME IN ROOT %s\n", de.filename);
}

int remove_dir(char *dir) {
    // parse filepath and check if dir exist


    super_block* sb = find_super_block();

    int curr_inode = 0, prev_inode = curr_inode;
    int string_i = 0;
    char filename[27];

    while(dir[string_i]!='\0'){        

        while(dir[string_i]=='/'){
            string_i++;
        }    
        int start = string_i, final=start;
        while(dir[string_i]!='/' && dir[string_i]!='\0'){
            string_i++;
        }
        final=string_i-1;

        memset(filename, '\0', 27);

        int i;
        for(i=0;i<26 && start+i<=final;i++){
            filename[i] = dir[start+i];
        }
        filename[i]='\0';
        //printf("%s \t %d\n", filename, curr_inode);

        prev_inode = curr_inode;
        curr_inode = find_file(sb, curr_inode, filename, 1);
        if(curr_inode<0){
            break;
        }
    }

    if(curr_inode < 0) {
        //printf("INVALID PATH\n");
        return -1;
    }

    delete_recursive(sb, curr_inode);

    // TODO set entry invalid        
    print_rootdir_data(sb, 0);    
    //printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");

    inode* __inode = find_inode(sb, prev_inode);

    int data_blocks = ceil(__inode->size, BLOCKSIZE);
    int b_index = 0;

    dir_entry *temp_de = (dir_entry *)malloc(sizeof(dir_entry));
    int flag = 0;
    int remaining_entries = __inode->size/32;
    while(b_index < data_blocks) {
        if(b_index < 5) {
            int d_block = __inode->direct[b_index];
            char* temp = (char *)malloc(BLOCKSIZE);
            read_block(global_diskptr, sb->data_block_idx + d_block, (void *)temp);

            for(int i=0;i<min(entries_per_block, remaining_entries) ;++i) {
                memcpy(temp_de, temp + i*32, 32);
                if(temp_de->inode == curr_inode) {
                    temp_de->meta_data &= ~(1);
                    flag = 1;

                    memcpy(temp + i*32, temp_de, 32);
                    write_block(global_diskptr, sb->data_block_idx + d_block, (void *)temp);
                    break;
                }
            }
            remaining_entries -= min(entries_per_block, remaining_entries);
            if(flag == 1) {
                break;
            }
        }
        else {
            int* x = (int *)malloc((BLOCKSIZE/4)*sizeof(int));
            read_block(global_diskptr, sb->data_block_idx + __inode->indirect, (void *)x);
            int d_block = x[b_index - 5];  

            char* temp = (char *)malloc(BLOCKSIZE);
            read_block(global_diskptr, sb->data_block_idx + d_block, (void *)temp);

            for(int i=0;i<min(entries_per_block, remaining_entries) ;++i) {
                memcpy(temp_de, temp + i*32, 32);;
                if(temp_de->inode == curr_inode) {
                    temp_de->meta_data &= ~(1);
                    flag = 1;

                    memcpy(temp + i*32, temp_de, 32);
                    write_block(global_diskptr, sb->data_block_idx + d_block, (void *)temp);
                    break;
                }
            }
            remaining_entries -= min(entries_per_block, remaining_entries);
            if(flag == 1) {
                break;
            }
        }
        b_index ++ ;
    }
    return 0;
}
