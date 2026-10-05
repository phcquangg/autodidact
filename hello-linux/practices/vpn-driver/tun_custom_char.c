#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

#define BUF_SIZE 2048
static char packet_buffer[BUF_SIZE];
static size_t packet_size = 0;

static ssize_t dev_read( struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	if (packet_size == 0) return 0;

	if (count > packet_size) count = packet_size;
	if (copy_to_user(buf, packet_buffer, count)) return -EFAULT;

	packet_size = 0;
	return count;
}

static ssize_t dev_write( struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	if (count > BUF_SIZE) count = BUF_SIZE;
	if (copy_from_user(packet_buffer, buf, count)) return -EFAULT;

	packet_size = count;
	return count;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = dev_read,
	.write = dev_write,
};

static struct miscdevice custom_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name =  "tun_custom",
	.fops = &fops,
};

static int __init char_init(void)
{
	return misc_register(&custom_miscdev);
}

static void __exit char_exit(void)
{
	misc_deregister(&custom_miscdev);
}

module_init(char_init);
module_init(char_exit);
MODULE_LICENSE("GPL");

