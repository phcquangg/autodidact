#define pr_fmt(fmt) "xorbox_dev:: " fmt

#include <linux/cdev.h>		// ops, dev, count, ... and cdev_[init | add | del]
#include <linux/init.h>		// module_init
#include <linux/module.h> // module_exit
#include <linux/kernel.h> // printk and stuff
#include <linux/types.h>	// dev_t
#include <linux/fs.h>			// major/minor numbers w alloc_chrdev_region ...
#include <linux/kdev_t.h> // MAJOR() & MINOR() macros
#include <linux/mutex.h>

#define DEV_COUNT 1

struct xorbox_dev
{
	char *buffer;
	size_t buffer_size;
	size_t data_len;
	size_t head;
	size_t tail;
	u8 key;
	struct mutex lock;
	struct cdev cdev;
};

static dev_t dev_number;

static struct xorbox_dev *cur_dev;

static int dev_open (struct inode *inode, struct file *filp);
static int dev_release (struct inode *inode, struct file *filp);
static ssize_t dev_read (struct file *filp, char __user *buffer, size_t len, loff_t *offset);
static ssize_t dev_write (struct file *filp, const char __user *buffer, size_t len, loff_t *offset);
static loff_t dev_llseek (struct file *filp, loff_t offset, int whence);
static long dev_unlocked_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);
static long dev_compat_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);

static struct file_operations fops = {
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
	.write = dev_write,
	.unlocked_ioctl = dev_unlocked_ioctl,
	.compat_ioctl = dev_compat_ioctl,
	.llseek = dev_llseek
};

static int __init xorbox_init(void)
{
	int allocated_number;
	allocated_number = alloc_chrdev_region(&dev_number, 0, DEV_COUNT, "dev_number");

	if (allocated_number < 0)
	{
		pr_err("Failed to allocate major number\n");
		return allocated_number;
	}

	pr_info("Number allocated\n");
	pr_info("allocated_number: %d\n", allocated_number);
	pr_info("dev_number: %U\n", dev_number);
	pr_info("Major number: %d\n", MAJOR(dev_number));
	pr_info("Minor number: %d\n", MINOR(dev_number));

	cur_dev = kmalloc(sizeof(struct xorbox_dev), GFP_KERNEL);
	if (!cur_dev)
	{
		pr_err("Failed to allocate device memory\n");
		return -ENOMEM;
	}

	mutex_init(&cur_dev->lock);

	cur_dev->buffer_size = 4096;
	cur_dev->buffer = kmalloc(cur_dev->buffer_size, GFP_KERNEL);

	if (!cur_dev->buffer)
	{
		pr_err("Failed to allocate buffer \n");
		return -ENOMEM;
	}

	cur_dev->head = 0;
	cur_dev->tail = 0;
	cur_dev->data_len = 0;
	cur_dev->key = 0x5A; // XOR key

	pr_info("Mutex & buffer initialized successfully\n");

	return 0;
}

static void __exit xorbox_exit(void)
{
	unregister_chrdev_region(dev_number, DEV_COUNT);
	pr_info("Device numbers unregistered\n");
}

MODULE_AUTHOR("Supa Quang");
MODULE_DESCRIPTION("Lorem Ipsum");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

module_init(xorbox_init);
module_exit(xorbox_exit);
