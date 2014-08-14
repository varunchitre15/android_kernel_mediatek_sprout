#
# for USB OTG silicon based on Mentor Graphics INVENTRA designs
#
include $(srctree)/drivers/misc/mediatek/Makefile.custom

ifeq ($(MTK_ALPS_BOX_SUPPORT),yes)
ccflags-y += -DMTK_ALPS_BOX_SUPPORT
endif

obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/

obj-$(CONFIG_USB_MTK_HDRC) += musb_hdrc.o

musb_hdrc-y := musb_core.o

musb_hdrc-y					+= musb_gadget_ep0.o musb_gadget.o
musb_hdrc-y					+= musb_virthub.o musb_host.o
musb_hdrc-y					+= musbhsdma.o 
musb_hdrc-$(CONFIG_USB_MTK_DEBUG_FS)		+= musb_debugfs.o
