#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include "sfs.h"

// extern disk* global_diskptr;

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
	int size = 200000;
	disk* new_disk = create_disk(size);
	if(new_disk == NULL) {
		printf("Error in creating disk\n");
		exit(-1);
	}
	int x = format(new_disk);
	if(mount(new_disk) < 0 ) {
		printf("Mount unsuccessfull\n");
		exit(-1);
	}

	printf("Mount successfull\n");
	// if(create_file() < 0 ) {
	// 	printf("File creation unsuccessfull\n");
	// 	exit(-1);
	// }
	// int Size = (BLOCKSIZE + BLOCKSIZE/2);
	// char* a = (char *)malloc(Size*sizeof(char));
	// int count = 0;
	// for(int i=0;i<Size;++i) {
	// 	a[i] = 'a' + i%26;
	// 	if(a[i] == 'a')
	// 		count ++;
	// }
	// printf("COUNT init: %d\n", count);
	// if(write_i(0, a, Size, 5*BLOCKSIZE + BLOCKSIZE/2) < 0) {
	// 	printf("Write Unsuccessfull\n");
	// }
	// read_i(0, a, Size, 5*BLOCKSIZE + BLOCKSIZE/2);
	// remove_file(0);
	// printf("####################################################################\n");
	char* filepath = "/dir1"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	printf("####################################################################\n");
	filepath = "/dir2"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	printf("####################################################################\n");
	filepath = "/dir3"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	printf("####################################################################\n");
	filepath = "/dir1/dir1.1"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	filepath = "/dir1/dir1.1/dir1.1.1"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	filepath = "/dir1/dir1.1/dir1.1.2"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	filepath = "/dir2/dir2.1"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	filepath = "/dir3/dir3.1"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	filepath = "/dir1/dir1.1"; 
	remove_dir(filepath);

	filepath = "/dir1/dir3.1"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	filepath = "/dir2/dir2.2"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	filepath = "/dir3/dir3.2"; 
	if(create_dir(filepath) < 0) {
		printf("Error in creation of directory");
	}

	// char* filepath = "/a.txt"; 
	// char *data = (char *)malloc(2);
	// data[0] = 'a';
	// data[1] = 'b';
	// if(write_file(filepath, data, 2, 0) < 0) {
	// 	printf("Error in creation of directory");
	// }

	// char *read = (char *)malloc(2);
	// filepath = "/a.txt"; 
	// if(read_file(filepath, read, 2, 0) < 0) {
	// 	printf("Error in creation of directory");
	// }
	// printf("READ DATA %c%c\n", read[0], read[1]);

	// filepath = "/a.txt"; 
	// char *data1 = (char *)malloc(3);
	// data1[0] = 'z';
	// data1[1] = 'y';
	// data1[2] = '\0';
	// if(write_file(filepath, data1, 3, 2) < 0) {
	// 	printf("Error in creation of directory");
	// }

	// char *read1 = (char *)malloc(5);
	// filepath = "/a.txt"; 
	// if(read_file(filepath, read1, 5, 0) < 0) {
	// 	printf("Error in creation of directory");
	// }
	// printf("READ DATA1 %s\n", read1);
	printf("END\n");
	return 0;
}