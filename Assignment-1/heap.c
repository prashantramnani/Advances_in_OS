#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT _IOR(0x10, 0x34, int32_t*)

#define READ_DATA _IOR('a', 0,int32_t*)
#define WRITE_DATA _IOW('a', 1, int32_t*)

#define min_heap 0xFF
#define max_heap 0xF0

static struct file_operations file_ops;

typedef struct pb2_set_type_arguments {
	int32_t heap_size;
	int32_t heap_type;
}set_type;

typedef struct obj_info {
	int32_t heap_size;
	int32_t heap_type;
	int32_t root;
	int32_t last_inserted;
}obj_info;


typedef struct process_heap_map {

	struct process_heap_map* next;
	struct process_heap_map* prev;
	pid_t pid;
	int32_t* heap;
	int32_t type;
	int32_t size;
	int32_t curr_num_elements;
	int32_t last_inserted;

} phm;

phm* HEAD = NULL;
phm* TAIL = NULL;

phm* new_heap(pid_t pid, int32_t type, int32_t size)
{
	phm* p = (phm *)kmalloc(sizeof(phm), GFP_KERNEL); 

	p->next = NULL;
	p->prev = NULL;
	p->pid = pid;
	p->type = type;
	p->size = size;
	p->curr_num_elements = 0;
	p->heap = (int32_t *)kmalloc(size*sizeof(int32_t), GFP_KERNEL);

	return p;
}

phm* heap_get_pid(pid_t pid)
{
	phm* H = HEAD;

	while(H!=NULL)
	{
		if(H->pid == pid)
			break;
		H = H->next;
	}
	return H;
}

void Swap(phm* H, int32_t a, int32_t b)
{
	int32_t temp = H->heap[a];
	H->heap[a] = H->heap[b];
	H->heap[b] = temp;
}

void heapify(phm* H, int32_t i)
{
	if(i == 0)
		return;
	int32_t parent = (i-1)/2;

	while(parent>0)
	{
		if(H->type)
		{
			if(H->heap[parent] < H->heap[i])
				Swap(H, parent, i);
		}
		else
		{
			if(H->heap[parent] > H->heap[i])
				Swap(H, parent, i);
		}

		i = parent;
		parent = (i-1)/2;
	}
	if(H->type)
	{
		if(H->heap[parent] < H->heap[i])
			Swap(H, parent, i);
	}
	else
	{
		if(H->heap[parent] > H->heap[i])
			Swap(H, parent, i);
	}
}

int insert_number(phm* H, int32_t* x)
{
	if(H->curr_num_elements == H->size)
	{
		return 0;
	}

	H->heap[H->curr_num_elements] = *x;
	H->curr_num_elements++;

	heapify(H, H->curr_num_elements-1);
	H->last_inserted = *x;
	return 1;
}

static char buffer[256] = {0};
static int buffer_len = 0;

static unsigned char type_size[2] ;

static int open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "FILE OPEN\n");
	pid_t pid = current->pid;

	if(heap_get_pid(pid))
	{
		printk(KERN_INFO "open() - File already open in Process: %d\n", pid);
		return -EACCES;
	}
	printk(KERN_INFO "open() - File open successfull for Process: %d\n", pid);
	return 0;
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) {
	printk(KERN_INFO "IN WRITE\n");

	if(!buf || !count) return -EINVAL;
    	if(copy_from_user(buffer, buf, count < 256 ? count:256)) return -ENOBUFS;
    	buffer_len = count < 256 ? count:256;
    	printk(KERN_INFO "%.*s", (int)count, buf);    return buffer_len;
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos) {
	printk(KERN_INFO "IN READ\n");
    	int ret = buffer_len;
    	if(!buffer_len) return 0;
    	if(!buf || !count) return -EINVAL;
    	if(copy_to_user(buf, buffer, buffer_len)) return -ENOBUFS;
    	printk(KERN_INFO "%.*s", (int)buffer_len, buffer);
    	buffer_len = 0;
	return ret;
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

    pid_t pid = current->pid;

    switch(cmd) {
        case READ_DATA:
	    printk(KERN_INFO "IN IOCTL READ\n");

            copy_to_user((char*) arg, buffer, 8);
            break;
	case WRITE_DATA:
	    printk(KERN_INFO "IN IOCTL WRITE\n");

    	    if(copy_from_user(type_size, (unsigned char *)arg, 2)) return -ENOBUFS;
    	    printk(KERN_INFO "%.*s\n", 2, type_size);
	    printk(KERN_INFO "%x\n", type_size[0]);

	    if(type_size[0] == min_heap)
	    {
		    printk(KERN_INFO "MIN_HEAP\n");
		    printk(KERN_INFO "HEAP SIZE %d\n", (int)type_size[1]);
	    }
	    else if(type_size[0] == max_heap)
	    {
		    printk(KERN_INFO "MAX_HEAP\n");
		    printk(KERN_INFO "HEAP SIZE %d\n", (int)type_size[1]);
	    }
            break;
	case PB2_SET_TYPE:
	    printk(KERN_INFO "IN PB2_SET_TYPE\n");
	    set_type* size_type = (set_type *)kmalloc(sizeof(set_type), GFP_KERNEL);

	    if(copy_from_user(size_type, (set_type *)arg, sizeof(set_type)))
	    {
		    printk(KERN_INFO "ERROR IN SETTING TYPE\n");
		    return -ENOBUFS;
	    }
	    printk(KERN_INFO "SUCCESS IN SETTING TYPE\n");
	    printk(KERN_INFO "Size %d Type %d \n", size_type->heap_size, size_type->heap_type);
	    
	   if(!HEAD)
	   {
		   HEAD = new_heap(pid, size_type->heap_type, size_type->heap_size);
		   TAIL = HEAD;
		   printk(KERN_INFO "HEAD %d %d\n", HEAD->size, HEAD->type);
	   }
	   else
	   {
		   phm* H = heap_get_pid(pid);

		   if(H)
		   {
			   kfree(H->heap);
			   H->type = size_type->heap_type;
			   H->size = size_type->heap_size;
			   H->curr_num_elements = 0;
			   H->heap = (int32_t *)kmalloc(H->size*sizeof(int32_t), GFP_KERNEL);
		   }
		   else
		   {
		   	   H = new_heap(pid, size_type->heap_type, size_type->heap_size);

			   TAIL->next = H;
			   H->prev = TAIL;
			   TAIL = H;
		   }
	   }
	   break;
	case PB2_INSERT:

	    printk(KERN_INFO "IN PB2_INSERT\n");
	    int32_t* x = (int32_t *)kmalloc(sizeof(int32_t), GFP_KERNEL);

	    if(copy_from_user(x, (int32_t *)arg, sizeof(int32_t)))
	    {
		    printk(KERN_INFO "ERROR IN RECEIVING NUMBER\n");
		    return -ENOBUFS;
	    }
	    printk(KERN_INFO "SUCCESS IN READING NUMBER\n");
	    printk(KERN_INFO "Number %d\n", *x);
	    
	    phm* H = heap_get_pid(pid);
	    int insert = insert_number(H, x);
	    
	    if(insert)
	    {
		    printk(KERN_INFO "Number Insert Success\n");
		    printk(KERN_INFO "Number Inserted: %d\n", H->heap[0]);
	    }
	    else
	    {
		    printk(KERN_INFO "Number Insert Failed\n");
	    }
	    

	    int i = H->curr_num_elements;
	    printk(KERN_INFO "Number of Elements in Heap: %d\n", H->curr_num_elements);
	    while(i)
	    {
		    printk(KERN_INFO "Elemnts in heap: %d\n",H->heap[i-1]);
		    i--;
	    }
	    break;
	case PB2_GET_INFO:

	    printk(KERN_INFO "IN PB2_GET_INFO\n");
	    obj_info* info = (obj_info *)kmalloc(sizeof(obj_info), GFP_KERNEL);

	    phm* h = heap_get_pid(pid);
	    info->heap_size = h->size;
	    info->heap_type = h->type;
	    info->root = h->heap[0];
	    info->last_inserted = h->last_inserted;

	    if(copy_to_user((obj_info *)arg, info, sizeof(obj_info)))
	    {
		    printk(KERN_INFO "ERROR IN GET INFO\n");
		    return -ENOBUFS;
	    }
	    printk(KERN_INFO "SUCCESS IN GET INFO\n");

	    break;
	case PB2_EXTRACT:
	    break;    
        default:
	    printk(KERN_INFO "FILE CLOSED\n");
        return -EINVAL;
    }
    return 0;
}


static int hello_init(void)
{
        struct proc_dir_entry *entry = proc_create("heap", 0, NULL, &file_ops);
        if(!entry) return -ENOENT;
        file_ops.owner = THIS_MODULE;
        file_ops.write = write;
	file_ops.read = read;
	file_ops.open = open;
	file_ops.unlocked_ioctl = ioctl;

        printk(KERN_ALERT "Hello, heap\n");
        return 0;
}

static void hello_exit(void)
{
        remove_proc_entry("heap", NULL);
        printk(KERN_ALERT "Nikal laude\n");
}

module_init(hello_init);
module_exit(hello_exit);
