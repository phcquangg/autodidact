ifeq ($(ARCH), arm)
	KDIR ?= $(HOME)/learnspace/imic/ti-processor-sdk-linux-am335x-evm-11.02.05.02/board-support/ti-linux-kernel-6.12.49+git-ti
else
	KDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
endif

obj-m += direct-led-blinking.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

