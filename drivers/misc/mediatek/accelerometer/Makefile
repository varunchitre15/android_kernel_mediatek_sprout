include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-y   +=  accel.o

ifeq ($(CONFIG_MTK_KXTJ2_1009),y)
obj-y	+=  KXTJ2_1009/
endif

ifeq ($(CONFIG_MTK_KXTIK1004),y)
obj-y	+=  kxtik1004/
endif

ifeq ($(CONFIG_MTK_K2DH),y)
obj-y	+=  k2dh/
endif

ifeq ($(CONFIG_MTK_BMA050-NEW),y)
obj-y   +=  bma050-new/
endif

ifeq ($(CONFIG_MTK_BMC156_ACC),y)
obj-y   +=  BMC156_ACC/
endif
ifeq ($(CONFIG_MTK_LSM303D),y)
obj-y   +=  lsm303d/
endif
