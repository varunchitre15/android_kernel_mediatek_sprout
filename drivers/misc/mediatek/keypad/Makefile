include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_MTK_KEYPAD) := kpd.o

ifeq ($(CONFIG_ARCH_MT6572), y)
  obj-y += mt6572/
endif
ifeq ($(CONFIG_ARCH_MT6582), y)
  obj-y += mt6582/
endif

