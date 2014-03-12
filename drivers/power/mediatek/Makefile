include $(srctree)/drivers/misc/mediatek/Makefile.custom

#ccflags-y += -I$(srctree)/drivers/power/mediatek/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)

obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter.o battery_common.o

ifneq ($(CONFIG_MTK_FAN5405_SUPPORT)_$(CONFIG_MTK_BQ24158_SUPPORT), _)
    obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o 
else
    obj-$(CONFIG_MTK_SMART_BATTERY) += linear_charging.o
endif

#obj-y	+= $(MTK_PLATFORM)/
