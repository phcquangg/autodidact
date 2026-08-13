#define pr_fmt(fmt) "xorbox_dev:: " fmt

#include <linux/cdev.h>		// ops, dev, count, ... and cdev_[init | add | del]
#include <linux/init.h>		// module_init
#include <linux/module.h> // module_exit
#include <linux/kernel.h> // printk and stuff
#include <linux/types.h>	// dev_t
#include <linux/fs.h>			// major/minor numbers w alloc_chrdev_region ...
#include <linux/kdev_t.h> // MAJOR() & MINOR() macros
#include <linux/mutex.h>
#include <linux/slab.h>		// kmalloc & kfree
#include <linux/uaccess.h> // copy_from_user & copy_to_user
#include <linux/device.h> // class_create & device_create

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

static struct class *xorbox_class = NULL;
static struct device *xorbox_device = NULL;
static dev_t dev_number;
static struct xorbox_dev *cur_dev;

static int dev_open (struct inode *inode, struct file *filp)
{
	struct xorbox_dev *dev;
	dev = container_of( inode->i_cdev, struct xorbox_dev, cdev);
	
	filp->private_data = dev;
	
	pr_info("Device opened successfully\n");
	return 0;
}

static int dev_release (struct inode *inode, struct file *filp)
{
	pr_info("Device closed successfully\n");
	return 0;
}

static ssize_t dev_read (struct file *filp, char __user *buffer, size_t len, loff_t *offset) 
{
	struct xorbox_dev *dev = filp->private_data;
	char kbuf[128];
	size_t bytes_to_read;
	size_t bytes_read;
	
	if (mutex_lock_interruptible(&dev->lock)) return -ERESTARTSYS;
	
	if (dev->data_len == 0) {
		mutex_unlock(&dev->lock);
		return 0;
	}

	bytes_to_read = min( len, dev->data_len);
	
	while (bytes_read < bytes_to_read) {
		size_t chunk_size = min(bytes_to_read - bytes_read, sizeof(kbuf));
		
		for (size_t i = 0; i < chunk_size; i++) {
			char cipher_byte = dev->buffer[dev->tail];
			
			kbuf[i] = cipher_byte ^ dev->key; // decrypt byte
			dev->tail = (dev->tail +1) % dev->buffer_size;
			dev->data_len--;
		}

		if (copy_to_user(buffer + bytes_read, kbuf, chunk_size)) {
			mutex_unlock(&dev->lock);
			return -EFAULT;
		}
		
		bytes_read+=chunk_size;
	}

	mutex_unlock(&dev->lock);

	pr_info("Read and decrypted %zu bytes\n", bytes_read);
	return bytes_read;
}
static ssize_t dev_write (struct file *filp, const char __user *buffer, size_t len, loff_t *offset)
{
	struct xorbox_dev *dev = filp->private_data;
	char kbuf[128];
	size_t bytes_to_write;
	size_t bytes_written = 0;

	if (mutex_lock_interruptible( &dev->lock)) return -ERESTARTSYS;
	
	size_t available_space = dev->buffer_size - dev->data_len;
	if (available_space == 0) {
		mutex_unlock(&dev->lock);
		return -ENOSPC;
	}

	bytes_to_write = min(len, available_space);
	
	while (bytes_written < bytes_to_write) {
		size_t chunk_size = min(bytes_to_write - bytes_written, sizeof(kbuf));

		if (copy_from_user(kbuf, buffer + bytes_written, chunk_size)) {
			mutex_unlock(&dev->lock);
			return -EFAULT;
		}
		
		for (size_t i = 0; i < chunk_size; i++) {
			char obfuscated_byte = kbuf[i] ^ dev->key;

			dev->buffer[dev->head] = obfuscated_byte;
			dev->head = (dev->head + 1) % dev->buffer_size; // circular wrap
			dev->data_len++;
		}

		bytes_written += chunk_size;
	}

	mutex_unlock(&dev->lock);
	pr_info("Wrote %zu bytes (XOR'd with key 0x%02X)\n", bytes_written, dev->key);

	return bytes_written;
}

static loff_t dev_llseek (struct file *filp, loff_t offset, int whence)
{
	pr_info("dev_llseek");
	return 0;
}

static long dev_unlocked_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	pr_info("dev_unlocked_ioctl");
	return 0;
}

static long dev_compat_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	pr_info("dev_compat_ioctl");
	return 0;
}

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
	int ret;

	allocated_number = alloc_chrdev_region(&dev_number, 0, DEV_COUNT, "dev_number");

	if (allocated_number < 0)
	{
		pr_err("Failed to allocate major number\n");
		return allocated_number;
	}

	pr_info("Number allocated\n");
	pr_info("allocated_number: %d\n", allocated_number);
	pr_info("dev_number: %u\n", dev_number);
	pr_info("Major number: %d\n", MAJOR(dev_number));
	pr_info("Minor number: %d\n", MINOR(dev_number));

	cur_dev = kmalloc(sizeof(struct xorbox_dev), GFP_KERNEL);
	if (!cur_dev)
	{
		pr_err("Failed to allocate device memory\n");

		ret = -ENOMEM;
		goto err_unregister;
	}

	mutex_init(&cur_dev->lock);

	cur_dev->buffer_size = 4096;
	cur_dev->buffer = kmalloc(cur_dev->buffer_size, GFP_KERNEL);

	if (!cur_dev->buffer)
	{
		pr_err("Failed to allocate buffer \n");

		ret = -ENOMEM;
		goto err_free_dev;
	}

	cur_dev->head = 0;
	cur_dev->tail = 0;
	cur_dev->data_len = 0;
	cur_dev->key = 0x5A; // XOR key

	pr_info("Mutex & buffer initialized successfully\n");

	// cdev
	cdev_init(&cur_dev->cdev, &fops);
	cur_dev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&cur_dev->cdev, dev_number, DEV_COUNT);
	
	if (ret < 0) {
		pr_err("Failed to add cdev\n");
		goto err_free_buffer;
	}

	/* * create /sys/class/xorbox ----  */
	xorbox_class = class_create("xorbox_class");
	if (IS_ERR(xorbox_class)) {
		pr_err("Failed to create class\n");
		ret = PTR_ERR(xorbox_class);
		goto err_del_cdev;
	}

	/* create /dev/xorbox automatically  */
	xorbox_device = device_create(xorbox_class, NULL, dev_number, NULL, "xorbox");
	if (IS_ERR(xorbox_device)) {
		pr_err("Failed to create device node\n");
		ret = PTR_ERR(xorbox_device);
		goto err_destroy_class;
	}

	return 0;

	err_destroy_class:
		class_destroy(xorbox_class);
	err_del_cdev:
		cdev_del(&cur_dev->cdev);
	err_free_buffer:
		kfree(cur_dev->buffer);
	err_free_dev:
		kfree(cur_dev);
	err_unregister:
		unregister_chrdev_region(dev_number, DEV_COUNT);
		return ret;	
}

static void __exit xorbox_exit(void)
{
	if (cur_dev) {
		cdev_del(&cur_dev->cdev);
		kfree(cur_dev->buffer);
		kfree(cur_dev);
	}

	if (xorbox_device) device_destroy(xorbox_class, dev_number);
	if (xorbox_class) class_destroy(xorbox_class);

	unregister_chrdev_region(dev_number, DEV_COUNT);
	pr_info("Device numbers unregistered\n");
}

MODULE_AUTHOR("Supa Quang");
MODULE_DESCRIPTION("XOR Obfuscation Pipe Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

module_init(xorbox_init);
module_exit(xorbox_exit);
