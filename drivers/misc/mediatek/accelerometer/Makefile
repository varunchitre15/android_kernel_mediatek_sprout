include $(srctree)/drivers/misc/mediatek/Makefile.custom

# In case the platform does NOT support this type of sensors
obj-y   +=  accelerometer_dummy.o

ifeq ($(CONFIG_MTK_KXTJ2_1009),y)
obj-y	+=  KXTJ2_1009/
endif

ifeq ($(CONFIG_MTK_K2DH),y)
obj-y	+=  k2dh/
endif

ifeq ($(CONFIG_MTK_BMA050-NEW),y)
obj-y   +=  bma050-new/
endif

