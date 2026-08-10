#undef PDEBUG
#ifdef SCULL_DEBUG
#	ifdef __KERNEL__
#	define PDEBUG(fmt, args...) printk( KERN_DEBUG "scull: " fmt, ## args)
#	else
#	define PDEBUG(fmt, args...) fprintf(stderr, fmt, ##args)
# 	endif
#else
# define PDEBUG(fmt, args...)
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...)

DEBUG = y

ifeq ($(DEBUG), y)
	DEBFLAGS = -O -g -DSCULL_DEBUG
else
	DEBFLAGS = -O2
endif

CFLAGS += $(DEBFLAGS)
