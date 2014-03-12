include $(srctree)/drivers/misc/mediatek/Makefile.custom

ccflags-y += -I$(MTK_PROJECT_PATH_ROOT)/touchpanel/msg2133/

# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/msg2133/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/


ifeq ($(CONFIG_MTK_PLATFORM),"mt6582")
    ccflags-y += -DMT6582
endif

obj-y	+=  msg2133_driver.o

