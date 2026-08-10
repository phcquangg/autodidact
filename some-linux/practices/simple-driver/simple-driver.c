#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>		// character device support
#include <linux/uaccess.h>	// copy_to_user and copy_from_user
#include <linux/slab.h>		// for kmalloc and kfree

MODULE_LICENSE("GPL");
MODULE_AUTHOR("");
MODULE_DESCRIPTION("A character device with dynamic buffering");
MODULE_VERSION("1.0");

#define DEVICE_NAME "my_simple_device"

static int buffer_size = 1024;
module_param(buffer_size, int, S_IRUGO);
MODULE_PARM_DESC(buffer_size, "Internal buffer capacity for the device");

static int major_number;
static char *kernel_buffer;
static int data_length = 0;

static int dev_open(struct inode *inodep, struct file *filep) {
	pr_info("%s: Device opened\n", DEVICE_NAME);
	return 0;
}

static ssize_t dev_read(struct file *filep, char __user * buffer, size_t len, loff_t *offset) {
	if (*offset > 0 || data_length == 0) {
		return 0;
	}

	if (len > data_length) {
		len = data_length;
	}

	if (copy_to_user(buffer, kernel_buffer, len) != 0) {
		pr_err("%s: Failed to send data to user\n", DEVICE_NAME);
		return -EFAULT;
	}

	*offset += len;
	pr_info("%s: Sent %zu bytes to user\n", DEVICE_NAME, len);
	return len;
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
	if (len > buffer_size) {
		len = buffer_size;
	}

	if (copy_from_user(kernel_buffer, buffer, len) != 0) {
		pr_err("%s: Failed to receive data from user\n", DEVICE_NAME);
		
		return -EFAULT;
	}

	data_length = len;
	pr_info("%s: Recieved %zu bytes from user\n", DEVICE_NAME, len);
	return len;
}

static int dev_release(struct inode *inodep, struct file *filep) {
	pr_info("%s: Device closed\n", DEVICE_NAME);
	return 0;
}

static struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release
};

static int __init m_init(void) {
	pr_info("%s: Initializing device with a buffer size of %d bytes\n", DEVICE_NAME, buffer_size);
	
	kernel_buffer = kmalloc(buffer_size, GFP_KERNEL);

	if (!kernel_buffer) {
		return -ENOMEM;
	}

	major_number = register_chrdev(0, DEVICE_NAME, &fops);

	if (major_number < 0) {
		pr_err("%s: Failed to register major number\n", DEVICE_NAME);
		kfree(kernel_buffer);
		return major_number;
	}

	pr_info("%s: Registered successfully with major number %d\n", DEVICE_NAME, major_number);
	return 0;
}

static void __exit m_exit(void) {
	unregister_chrdev(major_number, DEVICE_NAME);
	kfree(kernel_buffer);
	pr_info("%s: Cleaned up and removed module\n", DEVICE_NAME);
}

module_init(m_init);
module_exit(m_exit);

