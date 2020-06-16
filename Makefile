#ARCH=arm
#CROSS_COMPILE=arm-linux-gnueabihf-

obj-$(CONFIG_MY_CAL) += uio_modules.o
#KERNDIR=/mnt/nfs/Zedboard/Linux/linux-digilent
##KERNEL_VER=$(shell uname -r)
#
#default:
#	$(MAKE) -C $(KERNDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
#
#clean:
#	$(MAKE) -C $(KERNDIR) M=$(PWD) ARCH=$(ARCH) clean


