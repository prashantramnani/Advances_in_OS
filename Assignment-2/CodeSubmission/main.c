#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include "sfs.h"

void check_disk_functions(disk* diskptr) {
	int n= 10;
	int* a;
	a = (int *) malloc (n*(sizeof(int)));

	for(int i=0;i<n;++i) {
		a[i] = i;
	}

	write_block(diskptr, 0, (void *)a);

	int* b;
	b = (int *) malloc (n*(sizeof(int)));
	read_block(diskptr, 0, (void *)b);

	for(int i=0;i<n;++i) {
		if(b[i] != a[i]) {
			printf("Error in disk funcitons\n");
			exit(-1);
		}
	}
	if(free_disk(diskptr) < 0) {
		printf("Error in disk funcitons\n");
		exit(-1);
	}
	printf("Disk functions work!\n");
}

void total_size_check(disk* diskptr, int size) {
	int x = 0;
	x += sizeof(*(diskptr));
	for(int i=0;i<diskptr->blocks;++i) {
		x += 4*1024;
	}
	if(x == size) {
		printf("Size matches\n");
		return;
	}
	printf("Size does not matche\n");
}

int main(){
	disk* new_disk = (disk *)malloc(sizeof(disk));
	int size = 200000;
	if(create_disk(new_disk, size) < 0) {
		printf("Error in creating disk\n");
		exit(-1);
	}
	int x = format(new_disk);
	// print_superblock_info((super_block *)new_disk->block_arr[0]);
	return 0;
}