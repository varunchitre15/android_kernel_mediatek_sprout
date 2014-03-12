include $(srctree)/drivers/misc/mediatek/Makefile.custom

# In case the platform does NOT support this type of sensors
obj-y   +=  gyroscope_dummy.o

ifeq ($(CONFIG_MTK_MPU3050C),y)
obj-y	+=  mpu3050c/
endif

ifeq ($(CONFIG_MTK_MPU3000),y)
obj-y   +=  mpu3000/
endif

