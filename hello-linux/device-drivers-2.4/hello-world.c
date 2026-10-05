#include <linux/module.h>

MODULE_AUTHOR("SUPAQUANG");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

int init_module(void) { printk("<1>Hello, world\n"); return 0; }
void cleanup_module(void) { printk("<1>Goodbye cruel world\n"); }
