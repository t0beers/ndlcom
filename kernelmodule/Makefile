.EXPORT_ALL_VARIABLES:

TARGET = ndlcom
ARCH = arm
CROSS_COMPILE = arm-xilinx-linux-gnueabi-
KERNELDIR ?= ../../linux-xlnx

CC = $(CROSS_COMPILE)gcc
PWD := $(shell pwd)

obj-m := $(TARGET).o

default:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		CFLAGS="-O0" -C $(KERNELDIR) M=$(PWD) \
		modules

clean:
	rm -rf *.o *.ko $(TARGET).mod.c

distclean: clean
	rm -rf *~ Module.symvers .$(TARGET)* .tmp_versions modules.order
