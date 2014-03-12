# combo driver: MT6620
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifneq ($(KERNELRELEASE),)
	#COMBO_FOLDER := ../combo/common
	#ifneq ($(filter "MT6620E3",$(CONFIG_MTK_COMBO_CHIP)),)
	#	ccflags-y += -D MTK_WCN_COMBO_CHIP_SUPPORT
	#endif
	ifneq ($(filter "CONSYS_6572",$(CONFIG_MTK_COMBO_CHIP)),)
		ccflags-y += -D MTK_WCN_SOC_CHIP_SUPPORT
	endif
	ifneq ($(filter "CONSYS_6582",$(CONFIG_MTK_COMBO_CHIP)),)
		ccflags-y += -D MTK_WCN_SOC_CHIP_SUPPORT
	endif
	
	#ccflags-y += -I$(src)/$(COMBO_FOLDER)/linux/include -I$(src)/$(COMBO_FOLDER)/include -I$(src)/$(COMBO_FOLDER)/core/include
	
	CONN_COMMON := ../conn_soc/common
	ccflags-y += -I$(src)/$(CONN_COMMON)/linux/include
	
	obj-y += mtk_wcn_stub_alps.o
	obj-y += wmt_stp_exp.o

	obj-$(CONFIG_MTK_COMBO) += mtk_wmt_detect.o
	mtk_wmt_detect-objs := wmt_detect.o
	mtk_wmt_detect-objs += sdio_detect.o
	mtk_wmt_detect-objs	+= wmt_detect_pwr.o

# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD  := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
