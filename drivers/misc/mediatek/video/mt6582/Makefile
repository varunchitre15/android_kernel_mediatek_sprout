include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_MTK_FB) := dsi_drv.o dpi_drv.o lcd_drv.o disp_drv_dbi.o disp_drv_dpi.o disp_drv_dsi.o disp_hal.o disp_intr.o

ifeq ($(CONFIG_MTK_GPU_SUPPORT), y)
ccflags-y += -DHWGPU_SUPPORT
endif
