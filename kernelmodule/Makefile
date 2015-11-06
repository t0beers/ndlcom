.EXPORT_ALL_VARIABLES:

TARGET = ndlcom
ARCH = arm
CROSS_COMPILE = arm-xilinx-linux-gnueabi-
KERNELDIR ?= ../../linux-xlnx
XPARAMETERDIR ?= ../../ZynqBrain/ZynqBrain.sdk/fsbl_bsp/ps7_cortexa9_0/include

CC = $(CROSS_COMPILE)gcc
PWD := $(shell pwd)
EXTRA_CFLAGS := "-I$(XPARAMETERDIR)"

obj-m := $(TARGET).o

default:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
		-C $(KERNELDIR) M=$(PWD) \
		modules

clean:
	rm -rf *.o *.ko $(TARGET).mod.c

distclean: clean
	rm -rf *~ Module.symvers .$(TARGET)* .tmp_versions modules.order
