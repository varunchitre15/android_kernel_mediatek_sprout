# Display parameters shown under Settings -> About Phone

include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifeq ($(CONFIG_MTK_MMC),y)
  CONFIG_MTK_USIF = y
else
	ifeq ($(CONFIG_MTK_MTD_NAND),y)
  	CONFIG_MTK_USIF = y
  	else
  	CONFIG_MTK_USIF = n
  	endif
endif

obj-$(CONFIG_MTK_USIF) := DumChar.o
DumChar-objs :=  sysenv.o
