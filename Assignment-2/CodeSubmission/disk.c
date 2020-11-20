#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"

disk* create_disk(int nbytes){
	
	disk* diskptr = (disk *)malloc(sizeof(disk));
	// if(diskptr==NULL)
	// 	return -1;

	diskptr->size = 0;
	diskptr->blocks=0;

	int usable_size = nbytes - 24;
	if(usable_size < BLOCKSIZE){
		return NULL;
	}

	diskptr->size = nbytes;
	diskptr->blocks=usable_size/BLOCKSIZE;
	diskptr->reads = diskptr->writes = 0;
	diskptr->block_arr = (char **)malloc(diskptr->blocks*sizeof(char*));
	for(int i=0;i<diskptr->blocks;i++)
		diskptr->block_arr[i] = (char*)malloc(BLOCKSIZE*sizeof(char));

	return diskptr;
}

int read_block(disk *diskptr, int blocknr, void *block_data){

	if(diskptr==NULL)
		return -1;
	// Check if blocknr valid
	int B = diskptr->blocks;
	if(blocknr<0 || blocknr>=B){
		return -1;
	}

	// Copy data from diskptr->block_arr[blocknr] to block_data
	memcpy(block_data, diskptr->block_arr[blocknr], BLOCKSIZE);
	diskptr->reads++;
	return 0;
}

int write_block(disk *diskptr, int blocknr, void *block_data){

	if(diskptr==NULL)
		return -1;

	// Check if blocknr valid
	int B = diskptr->blocks;
	if(blocknr<0 || blocknr>=B){
		return -1;
	}

	// Copy data from diskptr->block_arr[blocknr] to block_data
	memcpy(diskptr->block_arr[blocknr], block_data, BLOCKSIZE);
	diskptr->writes++;
	return 0;
}

int free_disk(disk *diskptr){

	if(diskptr){
		for(int i=0;i<diskptr->blocks;i++)
			free(diskptr->block_arr[i]);
		free(diskptr->block_arr);
		free(diskptr);
		return 0;
	}
	return -1;
}

void print_disk_info(disk* diskptr) {
	printf("Information about the Disk\n");
	printf("Disk Size: %d\n", diskptr->size);
	printf("Number of Blocks: %d\n", diskptr->blocks);
	printf("Number of Reads: %d\n", diskptr->reads);
	printf("Number of Writes: %d\n", diskptr->writes);
}