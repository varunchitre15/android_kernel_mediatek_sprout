include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifeq ($(CONFIG_MTK_LCM_PHYSICAL_ROTATION), "90")
ccflags-y += -DMTK_LCM_PHYSICAL_ROTATION=\"90\"
endif

ifeq ($(CONFIG_MTK_LCM_PHYSICAL_ROTATION), "270")
ccflags-y += -DMTK_LCM_PHYSICAL_ROTATION=\"270\"
endif

ifeq ($(CONFIG_MTK_LCM_PHYSICAL_ROTATION), "180")
ccflags-y += -DMTK_LCM_PHYSICAL_ROTATION=\"180\"
endif

#ccflags-y += -I$(MTK_CUSTOM_PATH)/touchpanel/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/S3203/
#ccflags-y += -I$(MTK_CUSTOM_PATH)/touchpanel/

ccflags-y += -I$(MTK_PROJECT_PATH_ROOT)/touchpanel/S3203/

# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/arch/arm/mach-$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/S3203


ifeq ($(CONFIG_MTK_PLATFORM),"mt6572")
    ccflags-y += -DMT6572
endif

obj-y  +=  rmi_dev.o
obj-y  +=  rmi_fw_update.o
obj-y  +=  synaptics_rmi4_driver.o
