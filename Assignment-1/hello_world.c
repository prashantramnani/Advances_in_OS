#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");

static struct file_operations file_ops;

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) {
	printk("%.*s", count, buf); return count;
}


static int hello_init(void)
{
	struct proc_dir_entry *entry = proc_create("hello", 0, NULL, &file_ops);
	if(!entry) return -ENOENT;
	file_ops.owner = THIS_MODULE;
	file_ops.write = write;

	printk(KERN_ALERT "Hllo, world\n");
	return 0;
}

static void hello_exit(void)
{
	remove_proc_entry("hello", NULL);
	printk(KERN_ALERT "Nikal laude\n");
}

module_init(hello_init);
module_exit(hello_exit);
