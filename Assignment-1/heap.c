#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");

#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t*)
#define PB2_INSERT _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t*)
#define PB2_EXTRACT _IOR(0x10, 0x34, int32_t*)

#define READ_DATA _IOR('a', 0,int32_t*)
#define WRITE_DATA _IOW('a', 1, int32_t*)

#define min_heap 0xFF
#define max_heap 0xF0

static DEFINE_MUTEX(heap_mutex);


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

typedef struct result{
	int32_t result;
	int32_t heap_size;
} result;


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

void heapify_down(phm* H)
{
	int i=0;
	while(true)
	{
		int index = i;
		int left_child = 2*i + 1;
		int right_child = 2*(i+1);
		if(H->type)
		{
			if(left_child < H->curr_num_elements && H->heap[left_child] > H->heap[index])
				index = left_child;
			if(right_child < H->curr_num_elements && H->heap[right_child] > H->heap[index])
				index = right_child; 
		}
		else
		{
			if(left_child < H->curr_num_elements && H->heap[left_child] < H->heap[index])
				index = left_child;
			if(right_child < H->curr_num_elements && H->heap[right_child] < H->heap[index])
				index = right_child;
			
		}
		if(i!=index)
		{
			Swap(H,i,index);
			i=index;
		}
		else
			break;
	}
}

int remove_number(phm* H, int32_t* x)
{
	if(H->curr_num_elements == 0)
	{
		return 1;
	}
	*x = H->heap[0];
	H->heap[0] = H->heap[H->curr_num_elements-1];
	H->curr_num_elements--;

	// Heapify
	heapify_down(H);
	return 0;	
	
}


static char buffer[256] = {0};
static int buffer_len = 0;

static unsigned char type_size[2] ;

static int open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "FILE OPEN\n");
	pid_t pid = current->pid;
	while(!mutex_trylock(&heap_mutex));
	if(heap_get_pid(pid))
	{
		printk(KERN_INFO "open() - File already open in Process: %d\n", pid);
		mutex_unlock(&heap_mutex);
    	return -EACCES;
	}
	printk(KERN_INFO "open() - File open successfull for Process: %d\n", pid);
	mutex_unlock(&heap_mutex);
    return 0;
}

static int release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "FILE RELEASE\n");
	pid_t pid = current->pid;
	while(!mutex_trylock(&heap_mutex));
	phm* h = heap_get_pid(pid);
	if(h)
	{
		phm* left = h->prev;
		phm* right = h->next;
		if(left)
			left->next = right;
		else
			HEAD = right;
		if(right)
			right->prev= left;
		else
			TAIL = left;
		kfree(h->heap);
		kfree(h);
		printk(KERN_INFO "close() - File closed successfully for Process: %d\n", pid);
		mutex_unlock(&heap_mutex);
    	return 0;
	}
	printk(KERN_INFO "close() - File not open by Process: %d\n", pid);
	mutex_unlock(&heap_mutex);
    return 0;
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) {
	printk(KERN_INFO "IN WRITE\n");

	if(!buf || !count) return -EINVAL;
    pid_t pid = current->pid;
    while(!mutex_trylock(&heap_mutex));
	phm* H = heap_get_pid(pid);
	if(H)
	{
		//INSERT NEW ELEMENTS
		printk("INSERTING %s",buf);
		int32_t number = *(int32_t *)buf;
		printk(": %d\n",number);
		int insert = insert_number(H, &number);
	    
	    if(insert)
	    {
		    printk(KERN_INFO "Number Insert Success\n");
		    printk(KERN_INFO "Number Inserted: %d\n", H->heap[0]);
		    int i = H->curr_num_elements;
			printk(KERN_INFO "Number of Elements in Heap: %d\n", H->curr_num_elements);
			while(i)
			{
				printk(KERN_INFO "Elemnts in heap: %d\n",H->heap[i-1]);
				i--;
			}
			mutex_unlock(&heap_mutex);
		    return 4;
	    }
	    else
	    {
		    printk(KERN_INFO "Number Insert Failed\n");
		    mutex_unlock(&heap_mutex);
		    return -EACCES;
	    }
	    
	}
	else
	{
		//INITIALISE THE Node
		unsigned char mode = (unsigned char)buf[0];
		int32_t type;
		if(mode == min_heap)
			type=0;
		else if(mode == max_heap)
			type=1;
		else
		{
			//ERROR in HEAP TYPE
			mutex_unlock(&heap_mutex);
    		return -EINVAL;
		}
		
		int32_t size = (int32_t)buf[1];
		if(size<1 || size>100)
		{
			mutex_unlock(&heap_mutex);
    		return -EINVAL;	
		}
		if(!HEAD)
		{
		   HEAD = new_heap(pid, type, size);
		   TAIL = HEAD;
		   printk(KERN_INFO "HEAD %d %d\n", HEAD->size, HEAD->type);
		}
		else
		{
		   //phm* H = heap_get_pid(pid);

		   if(H)
		   {
			   kfree(H->heap);
			   H->type = type;
			   H->size = size;
			   H->curr_num_elements = 0;
			   H->heap = (int32_t *)kmalloc(H->size*sizeof(int32_t), GFP_KERNEL);
		   }
		   else
		   {
		   	   H = new_heap(pid, type, size);
			   TAIL->next = H;
			   H->prev = TAIL;
			   TAIL = H;
		   }
		}
		mutex_unlock(&heap_mutex);
    	return 2;
    }
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos) {
	printk(KERN_INFO "IN READ\n");
	pid_t pid = current->pid;
	while(!mutex_trylock(&heap_mutex));
	phm* H = heap_get_pid(pid);
	if(H)
	{
		// Heap exists
		int32_t n;
	    	if(!remove_number(H,&n))
	    	{
	    		mutex_unlock(&heap_mutex);
	    		char bytestream[4];
	    		bytestream[0] = n & 0xFF;
	    		bytestream[1] = (n>>8) & 0xFF;
	    		bytestream[2] = (n>>16) & 0xFF;
	    		bytestream[3] = (n>>24) & 0xFF;
	    		if(copy_to_user(buf,bytestream,count))
	    			return -ENOBUFS;
	    		printk(KERN_INFO "READ : %s %d \n", buf, n);
			return count;
	    	}
	    
		mutex_unlock(&heap_mutex);	
		return -EACCES;
	}
	else
	{
		// Heap DNE
		mutex_unlock(&heap_mutex);
		printk(KERN_INFO "NO ENTRY FOR PROCESS : %d\n",pid);
		return -EACCES;
	}
	
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

    pid_t pid = current->pid;
    while(!mutex_trylock(&heap_mutex));
    phm* H = heap_get_pid(pid);
	mutex_unlock(&heap_mutex);
    switch(cmd) {
    
	case PB2_SET_TYPE:
	    printk(KERN_INFO "IN PB2_SET_TYPE\n");
	    set_type* size_type = (set_type *)kmalloc(sizeof(set_type), GFP_KERNEL);

	    if(copy_from_user(size_type, (set_type *)arg, sizeof(set_type)))
	    {
		    printk(KERN_INFO "ERROR IN SETTING TYPE\n");
			kfree(size_type);    
		    return -ENOBUFS;
	    }
	    if((size_type->heap_type!=0 && size_type->heap_type!=1)||(size_type->heap_size < 1))
	    {
	    	kfree(size_type);
		    return -EINVAL;
	    }
	    printk(KERN_INFO "SUCCESS IN SETTING TYPE\n");
	    printk(KERN_INFO "Size %d Type %d \n", size_type->heap_size, size_type->heap_type);
	   
	   while(!mutex_trylock(&heap_mutex)); 
	   if(!HEAD)
	   {
		   HEAD = new_heap(pid, size_type->heap_type, size_type->heap_size);
		   TAIL = HEAD;
		   printk(KERN_INFO "HEAD %d %d\n", HEAD->size, HEAD->type);
	   }
	   else
	   {
		   //phm* H = heap_get_pid(pid);

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
	   mutex_unlock(&heap_mutex);
	   kfree(size_type);
	   return 0;
	   break;
	case PB2_INSERT:
	
	    printk(KERN_INFO "IN PB2_INSERT\n");
	    if(!H)
	    {
	    	return -EACCES;
	    }
	    	
	    int32_t* x = (int32_t *)kmalloc(sizeof(int32_t), GFP_KERNEL);
	    	
	    if(copy_from_user(x, (int32_t *)arg, sizeof(int32_t)))
	    {
			kfree(x);
		    printk(KERN_INFO "ERROR IN RECEIVING NUMBER\n");
		    return -ENOBUFS;

	    }
	    
	    printk(KERN_INFO "SUCCESS IN READING NUMBER\n");
	    printk(KERN_INFO "Number %d\n", *x);
	    
	    //phm* H = heap_get_pid(pid);
		while(!mutex_trylock(&heap_mutex));
	    int insert = insert_number(H, x);
	    mutex_unlock(&heap_mutex);
    
	    kfree(x);
	    if(insert)
	    {
		    printk(KERN_INFO "Number Insert Success\n");
		    printk(KERN_INFO "Number Inserted: %d\n", H->heap[0]);
		    
		    int i = H->curr_num_elements;
	    	printk(KERN_INFO "Number of Elements in Heap: %d\n", H->curr_num_elements);
		    while(i)
	    	{
			    printk(KERN_INFO "Elemnts in heap: %d\n",H->heap[i-1]);
			    i--;
	    	}
		    return 0;
	    }
	    else
	    {
		    return -EACCES;
	    }
	    break;
	case PB2_GET_INFO:
		
	    printk(KERN_INFO "IN PB2_GET_INFO\n");
	    
	    if(!H)
	    {
	    	return -EACCES;
	    }
	    
	    obj_info* info = (obj_info *)kmalloc(sizeof(obj_info), GFP_KERNEL);

	    //phm* h = heap_get_pid(pid);
	    info->heap_size = H->curr_num_elements;
	    info->heap_type = H->type;
	    info->root = H->heap[0];
	    info->last_inserted = H->last_inserted;

	    if(copy_to_user((obj_info *)arg, info, sizeof(obj_info)))
	    {
	    	kfree(info);
		    printk(KERN_INFO "ERROR IN GET INFO\n");
		    return -ENOBUFS;
	    }
	    printk(KERN_INFO "SUCCESS IN GET INFO\n");
    	kfree(info);
    	return 0;
	    break;
	case PB2_EXTRACT:
	    printk(KERN_INFO "IN PB2_EXTRACT\n");
	    
	    if(!H)
	    {
	    	return -EACCES;
	    }
	    result* result_o = (result *)kmalloc(sizeof(result),GFP_KERNEL);
	    
	    //phm* h = heap_get_pid(pid);
	    if(H)
	    {
	    	int32_t n;
	    	while(!mutex_trylock(&heap_mutex));
	    	if(!remove_number(H,&n))
	    	{
	    		result_o->result = n;
	    		result_o->heap_size = H->curr_num_elements;
	    		if(copy_to_user((result *)arg, result_o, sizeof(result)))
			    {
			    	kfree(result_o);
			    	mutex_unlock(&heap_mutex);
				    printk(KERN_INFO "ERROR IN EXTRACT\n");
				    return -1;
			    }
				mutex_unlock(&heap_mutex);
		    	kfree(result_o);
	    		printk(KERN_INFO "SUCCESS IN GET INFO\n");
			return 0;
	    	}
	    	mutex_unlock(&heap_mutex);
	    }
	    kfree(result_o);
	    return -1;
	    break;    
        default:
			printk(KERN_INFO "FILE CLOSED\n");
		    return -EINVAL;
    }
    return 0;
}


static int hello_init(void)
{
        struct proc_dir_entry *entryA = proc_create("partb_1_17CS10048", 0777, NULL, &file_ops);
        struct proc_dir_entry *entryB = proc_create("partb_2_17CS10038", 0777, NULL, &file_ops);
        if(!entryA) return -ENOENT;
        if(!entryB) return -ENOENT;
        file_ops.owner = THIS_MODULE;
        file_ops.write = write;
	file_ops.read = read;
	file_ops.open = open;
	file_ops.release = release;
	file_ops.unlocked_ioctl = ioctl;

        printk(KERN_ALERT "Hello, heap\n");
        return 0;
}

static void hello_exit(void)
{
        remove_proc_entry("heapA", NULL);
        remove_proc_entry("heapB", NULL);
 		mutex_destroy(&heap_mutex);
        printk(KERN_ALERT "Nikal laude\n");
}

module_init(hello_init);
module_exit(hello_exit);
