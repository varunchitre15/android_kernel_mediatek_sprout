include $(srctree)/drivers/misc/mediatek/Makefile.custom
obj-y   +=  mag.o
ifeq ($(CONFIG_MTK_HSCDTD006),y)
obj-y	+=  hscdtd006/
endif

ifeq ($(CONFIG_MTK_AKM8963),y)
obj-y	+=  akm8963/
endif

ifeq ($(CONFIG_MTK_BMM050-NEW),y)
obj-y   +=  bmm050-new/
endif

ifeq ($(CONFIG_MTK_BMC156_MAG),y)
obj-y   +=  BMC156_MAG/
endif
ifeq ($(CONFIG_MTK_LSM303D),y)
obj-y   +=  lsm303d/
endif
