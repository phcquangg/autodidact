#define pr_fmt(fmt) KBUILD_MODNAME " " fmt"\n"

#include <linux/fs.h>		// file_operations & _chrdev_region
#include <linux/kdev_t.h>	// MAJOR() and MINOR() macros
#include <linux/cdev.h>		// to register the file_operations to the driver
#include <linux/device.h>	// class & device
#include <linux/err.h>		// IS_ERR & PTR_ERR

#define DEV_NAME	"supa_led"
#define CLASS_NAME	"supa_led"
#define BUF_SIZE	64

static dev_t			dev_num;
static struct class		*dev_class;

struct dev_data {
	struct cdev		driver_cdev;
	struct device	dev;
	char			hardware_buffer[BUF_SIZE];
};

struct dev_data *dd;

static int dev_open( struct inode *inodep, struct file *filp)
{
	pr_info("Opened w major: %d | minor: %d", imajor(inodep), iminor(inodep));
	if( filp->f_flags & O_NONBLOCK) pr_info("File opened in non-blocking mode");
	return 0;
}

static int dev_release( struct inode *inodep, struct file *filp)
{
	pr_info("Device closed");

	return 0;
}

static ssize_t dev_write( struct file *filp, const char __user *user, size_t len, loff_t *offp)
{
	/**
	*	A typical write() process may look like:
	*		- input validation: remove escape sequences
	*		- update *offp by number of bytes successfully written
	*		- careful w buffer overflow
	*		- return len of consummed bytes
	*/
	struct dev_data *dd = filp->private_data;
	char	kbuf[BUF_SIZE];
	size_t	bytes_to_cp;

	bytes_to_cp = min(len, (size_t)(BUF_SIZE - 1));

	// unsigned long copy_from_user( void *to, const void __user *from, unsigned long n);
	if (copy_from_user(kbuf, user, bytes_to_cp)) {
		pr_err("Failed to copy_from_user");
		return -EFAULT;
	}

	pr_info("byte written: %s", kbuf);
	kbuf[bytes_to_cp] = '\0';

	if (bytes_to_cp > 0 && (kbuf[bytes_to_cp - 1] == '\n' || kbuf[bytes_to_cp - 1] == '\r')) kbuf[bytes_to_cp - 1] = '\0';

	if (strcmp(kbuf, "supa_on") == 0 || strcmp(kbuf, "1") == 0) {
		pr_info("Action: turn da supa LED ON");
	} else if (strcmp(kbuf, "supa_off") == 0 || strcmp(kbuf, "0") == 0) {
		pr_info("Action: turn da supa LED OFF");
	} else {
		pr_warn("Command \"%s\"unrecognized!!", kbuf);
		return -EINVAL;
	}

	*offp += bytes_to_cp;
	return bytes_to_cp;
}

static ssize_t dev_read( struct file *filp, char __user *user, size_t len, loff_t *offp)
{
	struct dev_data *dd = filp->private_data;
	size_t bytes_to_read;
	char kbuf[BUF_SIZE];

	if (*offp > 0) return 0;

	bytes_to_read = min(len, (size_t)strlen(kbuf));

	// unsigned long copy_to_user( void __user *to, const void *from, unsigned long n);
	if (copy_to_user(user, kbuf, bytes_to_read)) {
		pr_err("Failed to copy_to_user");
		return -EFAULT;
	}

	*offp += bytes_to_read;

	pr_info("dev_read: %s - %zu bytes", kbuf, bytes_to_read);

	return bytes_to_read;
}

static void device_release( struct device *dev)
{
	return;
} 

static struct file_operations fops = {
	.open = dev_open,
	.write = dev_write,
	.read = dev_read,
	.release = dev_release
};

static int __init dlb_init( void)
{
	int ret;

	// void *kzalloc( size_t size, unsigned int __nocast gfp_flags);
	dd = kzalloc(sizeof(*dd), GFP_KERNEL);
	if (!dd) return -ENOMEM;

	// alloc_chrdev_region( dev_t *, unsigned, unsigned, const char *);
	ret = alloc_chrdev_region( &dev_num, 0, 1, DEV_NAME);
	if (ret < 0) {
		kfree(dd);
		pr_err("Failed to alloc_chrdev_region\n");
		return -1;
	}

	// void cdev_init( struct cdev *, const struct file_operations *);
	cdev_init(&dd->driver_cdev, &fops);
	dd->driver_cdev.owner = THIS_MODULE;

	// void device_initialize( struct device *dev);
	device_initialize( &dd->dev);

	dd->dev.class = dev_class;
	dd->dev.devt = dev_num;
	dd->dev.release = NULL;
	// name in /dev/ and /sys/
	dev_set_name(&dd->dev, DEV_NAME);
	// int cdev_device_add( struct cdev *cdev, struct device *dev);

	ret = cdev_device_add(&dd->driver_cdev, &dd->dev);

	if (ret) {
		put_device(&dd->dev);
		unregister_chrdev_region(dev_num, 1);
		return ret;
	}

	return 0;
}

static void __exit dlb_exit( void)
{
	// void unregsiter_chrdev_region( dev_t, unsigned);
	unregister_chrdev_region(dev_num, 1);
	pr_info("chrdev numbers unregistered");
	// void cdev_device_del( struct cdev *cdev, struct device *dev);
	cdev_device_del(&dd->driver_cdev, &dd->dev);
	pr_info("cdev_device_del successfully");

	pr_info("Module removed successfully");
}

MODULE_AUTHOR("supa quangg");
MODULE_DESCRIPTION("Blinking LED by modifying device file");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");

module_init(dlb_init);
module_exit(dlb_exit);
