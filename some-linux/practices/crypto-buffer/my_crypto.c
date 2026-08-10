/*
 
Crypto-buffer Driver

Description:
	When driver loaded, allocate 10Mb of RAM
	When user write data into /dev/my_crypto, encrypt them using XOR pattern (later update to AES)
	and decrypt them when user read

For learning purpose
	Building Linux module
	Kernel memory management
	XOR / AES encryption
	...
	
Compile and deploy to Beaglebone Black with TI SDK.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("crypto-buffer-dev");
MODULE_DESCRIPTION("A 10MB Virtual Crypto-Buffer Driver");

#define DEVICE_NAME "my_crypto"
#define CLASS_NAME "crypto_class"
#define BUFFER_SIZE (10 * 1024 * 1024)
#define XOR_KEY 0x5A


static int majorNumber;
static char *crypto_buffer = NULL;
static size_t data_size = 0;
static struct class* cryptoClass = NULL;
static struct device* cryptoDevice = NULL;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static loff_t dev_llseek(struct file *filep, loff_t offset, int whence);

static struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
	.llseek = dev_llseek,
};

static int __init crypto_init(void) {
	printk(KERN_INFO "CryptoBuffer: Initializing the CryptoBuffer LKM\n");

	// allocate 10MB of RAM using kvmalloc
	crypto_buffer = kvmalloc(BUFFER_SIZE, GFP_KERNEL);
	
	if (!crypto_buffer) {
		printk(KERN_ALERT "CryptoBuffer: Failed to allocate memory\n");
		return -ENOMEM;
	}

	memset(crypto_buffer, 0, BUFFER_SIZE);
	printk(KERN_INFO "CryptoBuffer: Successfully allocated memory\n");

	// register a major number dynamically
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	
	if (majorNumber < 0) {
		printk(KERN_ALERT "CryptoBuffer failed to register a major number \n");
	
		kvfree(crypto_buffer);
		return majorNumber;
	}

	// register the device class
	cryptoClass = class_create(CLASS_NAME);
	
	if (IS_ERR(cryptoClass)) {
		unregister_chrdev(majorNumber, DEVICE_NAME);
		kvfree(crypto_buffer);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(cryptoClass);
	}

	// register the device driver
	cryptoDevice = device_create(cryptoClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(cryptoDevice)) {
		class_destroy(cryptoClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		kvfree(KERN_ALERT "Failed to create the device\n");
		return PTR_ERR(cryptoDevice);
	}

	printk(KERN_INFO "CryptoBuffer: device class created correctly\n");
	return 0;
};

static void __exit crypto_exit(void) {
	device_destroy(cryptoClass, MKDEV(majorNumber, 0));
	class_unregister(cryptoClass);
	class_destroy(cryptoClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);

	if (crypto_buffer) {
		kvfree(crypto_buffer);
	}

	printk(KERN_INFO "CryptoBuffer: Goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep) {
	return 0;
};

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
	size_t i;
	int errors = 0;
	char *temp_buf;

	if (*offset >= data_size) {
		return 0;
	}

	// adjust read length if exceeding available data
	if (len > data_size - *offset) {
		len = data_size - *offset;
	}

	// allocate a temp buffer to decrypt data before sending to user space
	temp_buf = kmalloc(len, GFP_KERNEL);
	if (!temp_buf) return -ENOMEM;

	// deccrypt data on the fly (XOR operation)
	for (i = 0; i < len; ++i) {
		temp_buf[i] = crypto_buffer[*offset + i] ^ XOR_KEY;	
	}

	// copy the decrypted data back to user space
	errors = copy_to_user(buffer, temp_buf, len);
	kfree(temp_buf);

	if (errors == 0) {
		*offset += len;
		return len;
	} else {
		return -EFAULT; // failed to copy data to user
	}
};


static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
	size_t i;
	int errors = 0;
	char *temp_buf;

	// prevent overflow
	if (*offset + len > BUFFER_SIZE) {
		return -EFBIG;
	}

	// allocate temp buffer for incoming user data
	temp_buf = kmalloc(len, GFP_KERNEL);
	if (!temp_buf) return -ENOMEM;

	// copy data from user space to temp_buf
	errors = copy_from_user(temp_buf, buffer, len);
	if (errors != 0) {
		kfree(temp_buf);
		return -EFAULT;
	}

	// encrypt data on the fly
	for (i = 0; i < len; ++i) {
		crypto_buffer[*offset + i] = temp_buf[i] ^ XOR_KEY;
	}

	printk(KERN_INFO "CryptoBuffer: Encrypted raw bytes in RAM:\n");
        print_hex_dump(KERN_INFO, "  RAW HEX: ", DUMP_PREFIX_OFFSET, 
                       16, 1, &crypto_buffer[*offset], len, false);

	kfree(temp_buf);
	*offset += len;

	if (*offset > data_size) {
		data_size = *offset;
	}

	return len;
};

static int dev_release(struct inode *inodep, struct file *filep) {
	return 0;
};

static loff_t dev_llseek(struct file *filep, loff_t offset, int whence) {
	loff_t new_pos = 0;

	switch (whence) {
		case 0:
			new_pos = offset;
			break;
		case 1:
			new_pos = filep->f_pos + offset;
			break;
		case 2:
			new_pos = data_size + offset;
			break;
		default:
			return -EINVAL;

	}

	if (new_pos < 0) return -EINVAL;
	if (new_pos > BUFFER_SIZE) return -EINVAL;
	
	filep->f_pos = new_pos;
	return new_pos;
	
}

module_init(crypto_init);
module_exit(crypto_exit);

