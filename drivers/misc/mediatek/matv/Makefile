include $(srctree)/drivers/misc/mediatek/Makefile.custom

# In case the platform does NOT support this type of sensors
obj-y   +=  matv_dummy.o

ifeq ($(CONFIG_MTK_MT5193),y)
obj-y	+=  mt5193/
endif
