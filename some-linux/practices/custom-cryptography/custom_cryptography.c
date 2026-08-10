#define pr_fmt(fmt) "xorbox_dev:: " fmt

#include <linux/cdev.h>     // ops, dev, count, ... and cdev_[init | add | del]
#include <linux/init.h>     // module_init
#include <linux/module.h>   // module_exit
#include <linux/kernel.h>   // printk and stuff
#include <linux/types.h>	// dev_t
#include <linux/fs.h>       // major/minor numbers w alloc_chrdev_region ...
#include <linux/kdev_t.h>   // MAJOR() & MINOR() macros

#define DEV_COUNT 1

struct xorbox_dev {
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

static int __init xorbox_init (void)
{
    int allocated_number;
    allocated_number = alloc_chrdev_region(&dev_number, 0, DEV_COUNT, "dev_number");

    if (allocated_number < 0) {
        pr_err("Failed to allocate major number\n");
        return allocated_number;
    }

    pr_info("Number allocated\n");
    pr_info("allocated_number: %d\n", allocated_number);
    pr_info("dev_number: %pR\n", dev_number);
    pr_info("Major number: %d\n", MAJOR(dev_number));
    pr_info("Minor number: %d\n", MINOR(dev_number));

    return 0;
}

static void __exit xorbox_exit (void)
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

