#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT _IOR(0x10, 0x34, int32_t*)

#define READ_DATA _IOR('a', 0,int32_t*)
#define WRITE_DATA _IOW('a', 1, int32_t*)

static struct pb2_set_type_arguments {
        int32_t heap_size;
        int32_t heap_type;
};

static struct obj_info {
        int32_t heap_size;
        int32_t heap_type;
	int32_t root;
	int32_t last_inserted;
};

int main()
{
        int fd = open("/proc/heap", O_RDWR);
        if(fd < 0)
	{
		printf("ERROR IN OPENING PROC FILE %d\n", fd);
		return 1;
	}	
        //unsigned char heap_init[] = {0xF0, 0x0C};
        //printf("size of msg %d\n", sizeof(heap_init));
        //ioctl(fd, WRITE_DATA, &heap_init);
	
	// OPENING A FILE	
	struct pb2_set_type_arguments size_type;
	size_type.heap_size = 20;
	size_type.heap_type = 0;
	printf("struct %d %d\n", size_type.heap_size, size_type.heap_type);

	ioctl(fd, PB2_SET_TYPE, &size_type);
	
	// RE-OPENING A FILE
	int ffd = open("/proc/heap", O_RDWR);
        if(ffd < 0)
	{
		printf("ERROR IN OPENING PROC FILE %d\n", ffd);
		//return 1;
	}	
	
	size_type.heap_size = 69;
	size_type.heap_type = 0;
	printf("struct %d %d\n", size_type.heap_size, size_type.heap_type);

	ioctl(fd, PB2_SET_TYPE, &size_type);


	//INSERTING NUMBERS IN THE HEAP
	int32_t x = 12;
	printf("Writing Number: %d\n", x);
	ioctl(fd, PB2_INSERT, &x);
	
	x = 20;
	printf("Writing Number: %d\n", x);
	ioctl(fd, PB2_INSERT, &x);

	x = 30;
	printf("Writing Number: %d\n", x);
	ioctl(fd, PB2_INSERT, &x);

	x = 8;
	printf("Writing Number: %d\n", x);
	ioctl(fd, PB2_INSERT, &x);

	struct obj_info info;
	ioctl(fd, PB2_GET_INFO, &info);

	printf("%d %d %d %d\n", info.heap_size, info.heap_type, info.root, info.last_inserted);

	close(fd);
}
