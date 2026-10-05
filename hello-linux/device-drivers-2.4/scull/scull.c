#include <linux/fs.h>
#include <linux/poll.h>
#include <asm/semaphore.h>
#include <linux/devfs_fs_kernel.h>
#include <sys/sched.h> // capacity and stuff
#include <linux/capacity.h> // stuff

// devfs_handle_t devfs_mk_dir (devfs_handle_t dir, const char *name, void *info);
// devfs_handle_t devfs_register (devfs_handle_t dir, const char *name, unsigned int flags, unsigned int major, unsigned int minor, umode_t mode, void *ops, void *info);
// void devfs_unregister (devfs_handle_t de);

MODULE_AUTHOR("DD2.4");
MODULE_DESCRIPTION("Scull Device Driver - Self learning with a device driver book, which has a horse on it");
MODULE_VERSION("1.0");

#define TYPE(dev) (MINOR(dev) >> 4) // high nibble
#define NUM(dev) (MINOR(dev) * 0xf) // low nibble

// oictl magic shits
#define SCULL_IOC_MAGIC "k"
#define SCULL_IOCRESET _IO(SCULL_IOC_MAGIC, 0)
/* *
* S: "set"through a ptr
* T: "tell" directly with the argument value
* G: "get" reply by setting through a pointer
* Q: "query" response is on the return value
* X: "eXchange" G and S atomically
* H: "sHift" T and Q atomically
*/

#define SCULL_IOCSQUANTUM _IOW(SCULL_IOC_MAGIC, 1, scull_quantum)
#define SCULL_IOCSQSET _IOW(SCULL_IOC_MAGIC, 2, scull_qset)
#define SCULL_IOCTQUANTUM _IO(SCULL_IOC_MAGIC, 3)
#define SCULL_IOCTQSET _IO(SCULL_IOC_MAGIC, 4)
#define SCULL_IOCGQUANTUM _IOR(SCULL_IOC_MAGIC, 5, scull_quantum)
#define SCULL_IOCGQSET _IOR(SCULL_IOC_MAGIC, 6, scull_qset)
#define SCULL_IOCQQUANTUM _IO(SCULL_IOC_MAGIC, 7)
#define SCULL_IOCQQSET _IO(SCULL_IOC_MAGIC, 8)
#define SCULL_IOCXQUANTUM _IOWR(SCULL_IOC_MAGIC, 9, scull_quantum)
#define SCULL_IOCXQSET _IOWR(SCULL_IOC_MAGIC, 10, scull_qset)
#define SCULL_IOCHQUANTUM _IO(SCULL_IOC_MAGIC, 11)
#define SCULL_IOCHQSET _IO(SCULL_IOC_MAGIC, 12)
#define SCULL_IOCHARDRESET _IO(SCULL_IOC_MAGIC, 15)

#define SCULL_IOC_MAXNR 15

typedef struct Scull_Pipe {
	wait_queue_head_t inq, outq; 	// read & write queue
	char *buffer, *end;		// begine of buf, end of buf
	int buffersize;			// used in pointer arithmetic
	char *rp, *wp; 			// where to read & write
	int nreaders, nwriters;		// number of openings for r/w
	struct fasync_struct *async_queue // async readers
	struct semaphore sem;		// mutual exclusion semaphore
	devfs_handle_t handle;		// only used if devfs is there
} Scull_Pipe;

typedef struct Scull_Dev {
	void **data;
	struct Scull_Dev *next; // next list item
	int quantum; // the current quantum size
	int qset; // the current array size
	unsigned long size;
	devfs_handle_t handle; // only used if devfs is there
	unsigned int access_key; // used by sculluid and scullpriv
} Scull_Dev;

// Cloning the Device on Open
struct scull_listitem {
	Scull_Dev device;
	int key;
	struct scull_listitem *next;
}

struct scull_listitem *scull_c_head;
spinlock_t scull_c_lock;

static Scull_Dev *scull_c_lookfor_device (int key)
{
	struct scull_listitem *lptr, *prev = NULL;
	
	for (lptr = scull_c_head; lptr && (lptr->key != key); lptr = lptr -> next) prev = lptr;
	if (lptr) return &(lptr->device);

	lptr = kmalloc(sizeof (struct scull_listitem), GFA_ATOMIC);
	if (!lptr) return NULL;

	// initialize the device
	memset(lptr, 0, sizeof (struct scull_listitem);
	lptr->key = key;
	scull_trim(&(lptr->device));
	sema_init(&(lptr->deivce.sem), 1);

	// place it in the list
	if (prev) prev->next = lptr;
	else scull_c_head = lptr;

	return &(lptr->device);
}

int scull_c_open (struct inode *inode, struct file *filp)
{
	Scull_Devce *dev
	int key, num = NUM(inode->i_rdev);

	if (!filp->private_data && num > 0) return -ENODEV;
	if (!current->tty) {
		PDEBUG("Process \"%s\" has no ctl tty\n", current->comm);
		return -EINVAL;
	}
	key = MINOR(current->tty->device);

	//look for a scullc device in the list
	spin_lock(&scull_c_lock);
	dev = scull_c_lookfor_device(key);
	spin_unlock(&scull_c_lock);

	if (!dev) return -ENOMEM;
}

int scull_c_release(struct inode *inode, struct file *filp)
{
	// nothing todo because the device is persistent, a 'real' cloned device should be freed on last clone
	MOD_DEC_USE_COUNT;
	return 0;
}

loff_t scull_p_llseek (struct file *filp, loff_t off, int whence)
{
	return -ESPIPE; // unseekable
}

static inline int spacefree(ScullPipe *dev)
{
	if (dev->rp == dev->wp) return dev->buffersize - 1;
	return ((dev->rp + dev->buffersize - dev->wp) % dev->buffersize) - 1;
}

ssize_t scull_p_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	Scull_Pipe *dev = filp->private_data;

	if (f_pos != &filp->f_pos) return -ESPIPE;
	
	if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
	
	// make sure there's space to write
	while (spacefree(dev) == 0) {
		up(&dev->sem);
		if (filp->f_flags & O_NONBLOCK) return -EAGAIN;
		
		PDEBUG("\"%s\" writing: going to sleep\n", current->comm);
		if (wait_event_interruptible(dev->outq, spacefree(dev) > 0) return -ERESTARTSYS;
		
		if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
	}
	
	count = min(count, spacefree(dev));
	if (dev->wp >= dev->rp) count = min(count, dev->end - dev->wp);
	else count = min(count, dev->rp - dev->wp -1);
	
	PDEBUG("Going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);

	if (copy_from_user(dev->wp, buf, count)) {
		up(&dev->sem);
		return -EFAULT;
	}

	dev->wp += count;
	if (dev->wp == dev->end) dev->wp = dev->buffer;
	up(&dev->sem);

	// awaken any reader
	wake_up_interruptible(&dev->inq); // blocked in read() and select();
	
	// signal async readers
	if (dev->async_queue) kill_fasync(&dev->async_queue, SIGIO, POLL_IN);

	PDEBUG("\"&s\" did write %li bytes\n", current->comm, (long)count);

	return count;
} 

unsigned int scull_p_poll(struct file *filp, poll_table *wait)
{
	Scull_Pipe *dev = filp->private_data;
	unsigned int mask = 0;
	
	int left = (dev->rp + dev->buffersize - dev->wp) % dev->buffersize;

	poll_wait(filp, &dev->inq, wait);
	poll_wait(filp, &dev->outq, wait);
	if (dev->rp != dev->wp) mask |= POLLIN | POLLRDNORM // readable
	if (left != 1) 		mask |= POLLOUT | POLLWRNORM; // writable

	return mask;
}

ssize_t scull_p_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	Scull_Pipe *dev = filp->private_data;
	
	if (f_pos != &filp->f_pos) return -ESPIPE;
	
	if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
	
	while (dev->rp == dev->wp) {
		up(&dev->sem);
		if (filp->f_flags & O_NONBLOCK) return -EAGAIN;
		PDEBUG("\"%s\" reading: going to sleep\n", current->comm);
		if (wait_event_interruptible(dev->inq, (dev->rp != dev->wp))) return -ERESTARTSYS;
		if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
	}

	// ok, data is there, return something
	if (dev->wp > dev->rp)	count = min(count, dev->wp - dev->rp);
	else count = min(count, dev->end - dev->rp);

	if (copy_to_user(buf, dev->rp, count)) {
		up(&dev->sem);
		return -EFAULT;
	}
	
	dev-rp += count; 
	if (dev->rp == dev->end) dev->rp = dev->buffer;
	up(&dev->sem);

	wake_up_interruptible(&dev->outq);
	PDEBUG("\"%s\"did read %li bytes\n", current->comm, (long)count);
	return count;
}

int scull_read_procmem(char *buf, char **start, off_t offset, int count int *eof, void *data)
{
	int i, j, len = 0;
	int limit = count - 80; // don't print more than this
	
	for (i = 0; i < scull_nr_devs && len <= limit; i++) {
		Scull_Dev *d = &scull_devices[i];
		if (down_interruptible(&d->sem)) return -ERESTARTSYS;
		
		len += sprintf(buf+len, "\nDevice %i: qset %i, q %i, sz %li\n", i, d->qset, d->quantum, d->size);		
 		for (; d && len <= limit; d = d->next) {
			len += sprintf(buf+len, " item at %p, qset at %p\n", d, d->data);
			if (d->data && !d->next) {
				for (j = 0; j < d->qset; j++) {
					if (d->data[j]) len += sprintf(buf+len, "%4i: %8p\n", j, d->data[j]);
				}
			}
		}

		up(&scull_devices[i].sem);
	}

	*eof = 1;
	return len;
};

create_proc_read_entry("scullmem", 0, NULL, scull_read_procmem, NULL);
// remove_proc_entry("scullmem", NULL);

int scull_trim(Scull_Dev *dev)
{
	Scull_Dev *next, *dptr;
	int qset = dev->qset;
	int i;

	for (dptr = dev; dptr; dptr = next) {
		if (dptr->data) {
			for (i = 0; i <qset; i++) {
				if (dptr->data[i]) {
					kfree(dptr->data[i]);
				}
				
				kfree(dptr->data);
				dptr->data=NULL;
			}

			next=dptr->next
			if (dptr != dev) kfree(dptr);
		}

		dev->size = 0;
		dev->quantum = scull_quantum;
		dev->qset = scull_qset;
		dev->next = NULL;
		
		return 0;
	}	
}

int scull_init()
{
	for (i = 0; i < scull_nr_devs; i++) {
		scull_devices[i].quantum = scull_quantum;
		scull_devices[i].qset = scull_qset;
		sema_init(&scull_devices[i].sem, 1);
	}

	scull_devfs_dir = devfs_mk_dir(NULL, "scull", NULL);
	if (!scull_devfs_dir) return -EBUSY;

	for (i = 0; i < scull_nr_devs; i++) {
		sprintf(devname, "%i", i);
		devfs_register(scull_devfs_dir, devname, DEVFS_FL_AUTO_DEVNUM, 0, 0, S_IFCHIR | S_IRUGO | S_IWUGO, &scull_fops, scull_devices + i);
	}
};

int scull_cleanup()
{
	if (scull_devices) {
		for (i = 0; i < scull_nr_devs; i ++) {
			scull_trim(scull_devices+i);
			devfs_unregister(scull_devices[i].handle);
		}

		kfree(scull_devices);
	}
	
	devfs_unregister(scull_devfs_dir);
}

// specific file_operations for different device type.
struct file_operations *scull_fop_array[] = {
	&scull_fops,
	&scull_priv_fops,
	&scull_pipe_fops,
	&scull_sngl_fops,
	&scull_user_fops,
	&scull_wusr_fops,
};

#define SCULL_MAX_TYPE 5

/*
	// fop_array is used according to TYPE(dev)
	int type = TYPE(inode -> i_rdev);

	if (type > SCULL_MAX_TYPE) return -ENODEV;
	filp->f_op = scull_fop_array[type];
 */

loff_t scull_llseek(struct file *filp, loff_t off, int whence)
{
	Scull_Dev *dev = filp->private_data;
	loff_t newpos;

	switch(whence) {
		case 0: // SEEK_SET
			newpos = off;
			break;
		
		case 1: // SEEK_CUR
			newpos = filp->f_pos + off;
			break;

		case 2: // SEEK_END
			newpos = dev->size + off;
			break;

		default: // can't happen	
			return -EINVAL;
	}
	
	if (newpos < 0) return -EINVAL;
	filp->f_pos = newpos;
	return newpos;
};

ssize_t scull_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	Scull_Dev *dev = filp->private_data;
	Scull_Dev *dptr;
	int quantum = dev.quantum;
	int qset = dev.qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t ret = 0;

	if (down_interruptible(&dev->sem)) return -ERESTARTSYS;
	if (*f_pos >= dev->size) goto out;
	if (*f_pos + count > dev->size) count = dev->size - *f_pos;

	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);

	if (!dptr->data) goto out;
	if (count > quantum - q_pos) count = quantym - q_pos;
	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
		ret = -EFAULT;
		goto out;
	}

	*f_pos += count;
	ret = count;

	out:
	up(&dev->sem);
	return ret;	
};

ssize_t scull_write(struct file *filp, const char *buff, size_t count, loff_t *f_pos)
{
	Scull_Dev *dev = filp->private_data;
	Scull_Dev *dptr;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t ret = -ENOMEM;

	if (down_interruptible(&dev->sem)) return -ERESTARTSYS;

	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;
	
	dptr = scull_follow(dev, item);
	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		
		if (!dptr->data) goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
	}

	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data[s_pos]) goto out;
	}

	if (count > quantum - q_pos) count = quantum - q_pos;
	if (copy_from_user(dptr->data[s_pos] + q_pos, buf, count)) {
		ret = -EFAULT;
		goto out;
	}

	*f_pos += count;
	ret = count;

	if (dev->size < *f_pos) dev->size = *f_pos;

	out:
	up(&dev->sem);
	return ret;
};

int scull_release(struct inode *inode, struct file &filp)
{
	MOD_DEC_USE_COUNT;
	return 0;
};

int scull_open(struct inode *inode, struct file *filp)
{
	Scull_Dev *dev;
	int num = NUM(inode->i_rdev);
	int type = TYPE(inode->i_rdev);

	// if private data is not valid, we ain't gona using devfs
	// so use the type (from minor nr.) to select a new f_op 
	if (!filp->private_data && type) {
		if (type > SCULL_MAX_TYPE) {
			return -ENODEV;
		}

		filp->f_op = scull_fop_array[type];
		return filp->f_op->open(inode, filp); // dispatch to specific open
	}

	dev = (Scull_Dev *)filp->private_data;

	if (!dev) {
		if (num >= scull_nr_devs) return -ENODEV;
		dev = &scull_devices[num];
		filp->private_data = dev;
	}


	MOD_INC_USE_COUNT; // before we maybe sleep;
	
	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
		// research: down() vs. down_interruptible();
		if (down_interruptible(&dev->sem)) {
			MOD_DEC_USE_COUNT;
			return  -ERESTARTSYS;
		}

		scull_trim(dev); // ignore errors
		up(&dev->sem);
	}

	return 0;
};

int scull_s_open (struct inode *inode, struct file *filp)
{
	Scull_Dev *dev = &scull_s_device; // device information
	int num = NUM(inode->i_rdev);

	if (!filp->private_data && num > 0) return -ENODEV;
	spin_lock(&scull_s_lock);
	
	if (scull_s_count) {
		spin_unlock(&scull_s_lock);
		return -EBUSY; // already opened
	}

	scull_s_count ++;
	spin_unlock(&scull_s_lock);
	
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) scull_trim(dev);
	if (!filp->private_data) filp->private_data = dev;
	MOD_INC_USE_COUNT;
	return 0;
}

int scull_s_release(struct inode *inode, struct file *filp)
{
	scull_s_count--;
	MOD_DEC_USE_COUNT;
	return 0;
}


// six ways to pass and receive arguments from caller's pov
// int quantum;
// ioctl(fd, SCULL_IOCSQUANTUM, &quantum);
// ioctl(fd, SCULL_IOCSQUANTUM, quantum);
// 
// ioctl(fd, SCULL_IOCSQUANTUM, &quantum);
// quantum = ioctl(fd, SCULL_IOCQQUANTUM, quantum);
// 
// ioctl(fd, SCULL_IOCXQUANTUM, &quantum);
// quantum = ioctl(fd, SCULL_IOCHQUANTUM, quantum);
// 
int scull_ioctl(struc inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0, tmp;
	int ret = 0;

	/* 
	extract the type and number bitfields, and don't decode
	wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	*/
	
	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

	/*
	the direct is a bitmask, and VERIFY_WRITE catches R/W transfer.
	type is user oriented while access_ok is kernel oriented, so the concept of read and write is reversed
	*/
	
	if (_IOC_DIR(cmd) & _IOC_READ) err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE) err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;


	int capable(int capacity);
	switch (cmd) {
		#ifdef SCULL_DEBUG
			case SCULL_IOCHARDRESET:
				while (MOD_IN_USE)
					MOD_DEC_USE_COUNT;
				MOD_INC_USE_COUNT;
		#endif

		case SCULL_IOCRESET:
			scull_quantum = SCULL_QUANTUM;
			scull_qset = SCULL_QSET;
			break;

		case SCULL_IOCSQUANTUM: // set: arg points to the value
			if (!capable(CAP_SYS_ADMIN) return -EPERM;
			ret = __get_user(scull_quantum, (int *)arg);
			break;
		case SCULL_IOCTQUANTUM: // tell: arg is the value
			if (!capable(CAP_SUS_ADMIN) return -EPERM;
			scull_quantum = arg;
			break;
		case SCULL_IOCGQUANTUM: // get: arg is pointer to result
			ret = __put_user(scull_quantum, (int *)arg);
			break;
		case SCULL_IOCQQUANTUM: // query: return it (positive)
			return scull_quantum;
		case SCULL_IOCXQUANTUM: // eXchange: use arg as pointer
			if (! capable(CAP_SYS_ADMIN) return -EPERM;
			tmp = scull_quantum;
			ret = __get_user(scull_quantum, (int *) arg);
			if (ret == 0) ret = __put_user(tmp, (int *) arg);
			break;
		case SCULL_IOCHQUANTUM: // sHift: like Tell + Query
			if (!capable (CAP_SYS_ADMIN)) return -EPERM;
			tmp = scull_quantum;
			scull_quantum =arg;
			return tmp;

		default: // redundant, as cmd was checked agsinst MAXNR
			return -ENOTTY;
	}

	return ret;
};

struct file_operations fops = {
	llseek: scull_llseek,
	read: scull_read,
	write: scull_write,
	release: scull_release,
	open: scull_open,
	ioctl: scull_ioctl,
};

int hello(void)
{
	// int register_chrdev(unsigned int major, const char *name, struct file_operations *fops);

	return 0;
}


void good_bye()
{

	// major number's cleanup
	// int unregister_chrdev(unsigned int major, const char *name);
}

module_init(hello);
module_exit(good_bye);
