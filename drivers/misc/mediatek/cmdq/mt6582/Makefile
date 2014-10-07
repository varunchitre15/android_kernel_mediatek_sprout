# drivers/cmdq
include $(srctree)/drivers/misc/mediatek/Makefile.custom
ccflags-y += -I$(srctree)/drivers/misc/mediatek/dispsys/$(MTK_PLATFORM) \
             -I$(srctree)/drivers/misc/mediatek/smi/$(MTK_PLATFORM) \
             -I$(srctree)/drivers/misc/mediatek/cmdq

# Enable MET
ccflags-y += -DMET_USER_EVENT_SUPPORT

obj-y += cmdq_mdp.o
obj-y += cmdq_platform.o
obj-y += cmdq_device.o
