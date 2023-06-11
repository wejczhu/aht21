KERNELDIR := 
CURRENT_PATH := $(shell pwd)
CC ?= gcc
LD ?= ld

ifeq ($(CROSS_COMPILER),)
	COMPILER_CC := $(CC)
	COMPILER_LD := $(LD)
else
	COMPILER_CC := $(CROSS_COMPILER)-gcc
	COMPILER_LD := $(CROSS_COMPILER)-ld
endif

obj-m = aht21.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) ARCH=arm CC=$(COMPILER_CC) LD=$(COMPILER_LD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) ARCH=arm clean
