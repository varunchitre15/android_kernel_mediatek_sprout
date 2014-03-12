# drivers/video/mtk/Makefile
#
#

include $(srctree)/drivers/misc/mediatek/Makefile.custom
obj-$(CONFIG_MTK_FB) += disp_drv.o                                 \
         mtkfb_console.o                            \
         disp_assert_layer.o                        \
         mtkfb.o                                    \
		 mtkfb_vsync.o								\
         debug.o                                    \
         font_8x16.o                                \
         mtkfb_proc.o                               \
         disp_sync.o


#ifeq ($(MTK_DISP_CONFIG_SUPPORT), yes)
obj-$(CONFIG_MTK_FB) += fbconfig_kdebug.o
#endif

ccflags-y += -I$(MTK_PATH_SOURCE)/drivers/video/  \
			 -I$(MTK_PATH_PLATFORM)/drivers/video/  \
			 -I$(MTK_PATH_PLATFORM)/drivers/dispsys/ \
			 -I$(MTK_PATH_PLATFORM)/core/include/mach/ \
			 -I$(MTK_PATH_SOURCE)/drivers/sync/

ifeq ($(CONFIG_MTK_INTERNAL_HDMI_SUPPORT), y)
ccflags-y += -I$(MTK_PATH_PLATFORM)/drivers/hdmi/
endif  
ifeq ($(CONFIG_MTK_INTERNAL_MHL_SUPPORT), y)
ccflags-y += -I$(MTK_PATH_PLATFORM)/drivers/mhltx/
endif   
ifeq ($(CONFIG_MTK_DITHERING_SUPPORT), y)
ccflags-y += -DDITHERING_SUPPORT
endif

ifeq ($(CONFIG_MTK_GPU_SUPPORT), y)
ccflags-y += -DHWGPU_SUPPORT
endif

ifeq ($(CONFIG_MMPROFILE),y)
ccflags-y += -DDEFAULT_MMP_ENABLE
endif

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/

