include $(srctree)/drivers/misc/mediatek/Makefile.custom
obj-y   +=  gyroscope.o
ifeq ($(CONFIG_MTK_BMG160),y)
obj-y	+=  bmg160/
endif

ifeq ($(CONFIG_MTK_MPU3050C),y)
obj-y	+=  mpu3050c/
endif

ifeq ($(CONFIG_MTK_MPU3000),y)
obj-y   +=  mpu3000/
endif

ifeq ($(CONFIG_MTK_L3GD20),y)
obj-y   +=  l3gd20/
endif
