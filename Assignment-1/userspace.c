#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>


#define READ_DATA _IOR('a', 0,int32_t*)
#define WRITE_DATA _IOW('a', 1, int32_t*)


int main()
{
	int fd = open("/proc/heap", O_RDWR);
        if(fd < 0) return 1;
	printf("ksdbshdbv\n");
	char heap_init[] = {0xFF, '3'};
	printf("size of msg %d\n", sizeof(heap_init));
	ioctl(fd, WRITE_DATA, &heap_init);
	
		
        //int value, number;
	//char M[256];
        //ioctl(fd, READ_DATA, &M);
        //printf("Value is %s\n", M);

        close(fd); return 0;
}

