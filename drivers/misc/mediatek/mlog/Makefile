include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifeq ($(CONFIG_MTK_GPU_CHIP), "MALI400MP1")
ccflags-y += -DMALI_SUPPORT
ccflags-y += -I$(srctree)/drivers/misc/mediatek/gpu/$(MTK_PLATFORM)/mali/mali/common
ccflags-y += -I$(srctree)/drivers/misc/mediatek/gpu/$(MTK_PLATFORM)/mali/mali/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/gpu/$(MTK_PLATFORM)/mali/mali/linux
ccflags-y += -I$(srctree)/drivers/misc/mediatek/gpu/$(MTK_PLATFORM)/mali/mali/linux/license/gpl

endif

ccflags-$(CONFIG_ZRAM) += -I$(srctree)/drivers/staging/zram
ccflags-$(CONFIG_ZSMALLOC) += -I$(srctree)/drivers/staging/zsmalloc

obj-y += mlog.o
mlog-y := mlog_dump.o mlog_logger.o
