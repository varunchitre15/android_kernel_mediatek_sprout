# common driver code for CMDQ (ROME, K2~)
# 72/82/92 does not use this common code

include $(srctree)/drivers/misc/mediatek/Makefile.custom
ccflags-y += -I$(srctree)/drivers/misc/mediatek/cmdq/$(MTK_PLATFORM) \
             -I$(srctree)/drivers/misc/mediatek/dispsys/$(MTK_PLATFORM) \
             -I$(srctree)/drivers/misc/mediatek/smi/$(MTK_PLATFORM)

# Enable MET
ccflags-y += -DMET_USER_EVENT_SUPPORT

# driver module
obj-y += cmdq_record.o
obj-y += cmdq_core.o
obj-y += cmdq_driver.o
obj-y += cmdq_mmp.o
obj-y += cmdq_test.o
obj-y += cmdq_mdp_common.o

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/

