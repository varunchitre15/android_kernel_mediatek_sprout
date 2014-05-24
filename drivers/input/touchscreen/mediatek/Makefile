include $(srctree)/drivers/misc/mediatek/Makefile.custom


# In case the platform does NOT support this type of sensors
obj-y   +=  mtk_tpd.o
obj-y   +=  tpd_button.o
obj-y   +=  tpd_calibrate.o
obj-y   +=  tpd_debug.o
obj-y   +=  tpd_default.o
obj-y   +=  tpd_init.o
obj-y   +=  tpd_misc.o
obj-y   +=  tpd_setting.o

ifeq ($(CONFIG_MTK_GT9XX),y)
obj-y	+=  GT9XX/
endif

ifeq ($(CONFIG_MTK_MMS134),y)
obj-y	+=  mms134/
endif

ifeq ($(CONFIG_TOUCHSCREEN_SYNAPTICS_S2202), y)
obj-y   += synaptics_2202/
endif

ifeq ($(CONFIG_MTK_FT5406),y)
obj-y	+=  ft5406/
endif

ifeq ($(CONFIG_MTK_MSG2133),y)
obj-y	+=  msg2133/
endif

ifeq ($(CONFIG_MTK_SYNAPTICS_3508),y)
obj-y	+=  synaptics_3508/
endif

ifeq ($(CONFIG_MTK_FT6X06),y)
obj-y	+=  ft6x06/
endif

ifeq ($(CONFIG_MTK_S3203),y)
obj-y	+=  S3203/
endif
