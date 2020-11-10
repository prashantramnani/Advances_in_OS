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

int main(){
	disk* new_disk = (disk *)malloc(sizeof(disk));
	if(create_disk(new_disk, 4120) < 0) {
		printf("Error in creating disk\n");
		exit(-1);
	}
	print_disk_info(new_disk);
	check_disk_functions(new_disk);

	return 0;
}