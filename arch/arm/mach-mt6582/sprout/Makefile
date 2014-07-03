
include $(srctree)/drivers/misc/mediatek/Makefile.custom

# Module project related part

ifeq ($(CONFIG_MTK_CM36283),y)
obj-y	+=  alsps/cm36283/
endif

ifeq ($(CONFIG_MTK_TMD2771),y)
obj-y	+=  alsps/tmd2771/
endif

ifeq ($(CONFIG_MTK_BMA050-NEW),y)
obj-y   +=  accelerometer/bma050-new/
endif

ifeq ($(CONFIG_MTK_KXTIK1004),y)
obj-y   +=  accelerometer/kxtik1004/
endif

ifeq ($(CONFIG_MTK_BMC156_ACC),y)
obj-y   +=  accelerometer/bmc156_acc/
endif
ifeq ($(CONFIG_MTK_LSM303D),y)
obj-y   +=  accelerometer/lsm303d/
obj-y   +=  magnetometer/lsm303d/
endif
ifeq ($(CONFIG_MTK_MPU3000),y)
obj-y   +=  gyroscope/mpu3000/
endif
ifeq ($(CONFIG_MTK_L3GD20),y)
obj-y   +=  gyroscope/l3gd20/
endif
ifeq ($(CONFIG_MTK_MPU3050C),y)
obj-y   +=  gyroscope/mpu3050c/
endif

ifeq ($(CONFIG_MTK_BMG160),y)
obj-y   +=  gyroscope/bmg160/
endif
ifeq ($(CONFIG_MTK_BMM050-NEW),y)
obj-y	+=  magnetometer/bmm050-new/
endif
ifeq ($(CONFIG_MTK_BMC156_MAG),y)
obj-y	+=  magnetometer/bmc156_mag/
endif

ifneq ($(CONFIG_ARCH_MTK_PROJECT),)
ARCH_MTK_PROJECT := $(shell echo $(CONFIG_ARCH_MTK_PROJECT))
obj-y	+= core/
obj-y	+= dct/dct/
endif

obj-y   +=  lcm/
#obj-y	+=  hdmi/
obj-y	+=  camera/camera/
obj-y	+=  imgsensor/ov5648_mipi_raw/
obj-y	+=  imgsensor/ov5648_mipi_raw_truly/
obj-y	+=  imgsensor/hi258_mipi_yuv/
obj-y	+=  imgsensor/ov2680_mipi_raw/
obj-y	+=  imgsensor/ov2680_mipi_raw_kaimujin/
obj-y +=  flashlight/constant_flashlight/
obj-y	+=  vibrator/
obj-y	+=  leds/
obj-y	+=  keypad/
obj-y	+=  sound/
obj-y	+=  accdet/
obj-y	+=  wmt/
